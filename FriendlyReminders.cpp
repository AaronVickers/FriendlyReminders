#include "FriendlyReminders.h"

// Initialise plugin
BAKKESMOD_PLUGIN(FriendlyReminders, "Friendly Reminders", "0.3", PLUGINTYPE_FREEPLAY)

// Define CVARs
#define CVAR_SHOW_GOAL_MESSAGES "show_goal_messages"
#define CVAR_SHOW_GAME_FINISHED_MESSAGES "show_game_finished_messages"

#define CVAR_COMBINE_MESSAGES "combine_messages"

#define CVAR_PICK_MESSAGE_METHOD "pick_message_method"
#define CVAR_DISPLAY_MESSAGE_METHOD "display_message_method"

#define CVAR_GOAL_MESSAGES "goal_messages"
#define CVAR_GAME_FINISHED_MESSAGES "game_finished_messages"

// Load method
void FriendlyReminders::onLoad()
{
	// Register CVARs
	cvar_show_goal_messages = std::make_shared<bool>(true);
	cvar_show_game_finished_messages = std::make_shared<bool>(true);
	cvar_combine_messages = std::make_shared<bool>(false);
	cvar_pick_message_method = std::make_shared<std::string>("Random");
	cvar_display_message_method = std::make_shared<std::string>("Default");
	// TODO: Find a better way to implement a list of messages
	//cvar_goal_messages = std::make_shared<std::string>("Drink some water!,Check your posture!");
	//cvar_game_finished_messages = std::make_shared<std::string>("Do some push-ups!,Do some sit-ups");

	// Enabled status (boolean)
	cvarManager->registerCvar(CVAR_SHOW_GOAL_MESSAGES, "1", "Show messages when a goal is scored", true, true, 0, true, 1, true)
		.bindTo(cvar_show_goal_messages);

	cvarManager->registerCvar(CVAR_SHOW_GAME_FINISHED_MESSAGES, "1", "Show messages when a game is finished", true, true, 0, true, 1, true)
		.bindTo(cvar_show_game_finished_messages);

	// Combine message lists (boolean)
	cvarManager->registerCvar(CVAR_COMBINE_MESSAGES, "0", "Use both lists of messages for both message events", true, true, 0, true, 1, true)
		.bindTo(cvar_combine_messages);

	// Message picking method (Random, Indexed)
	cvarManager->registerCvar(CVAR_PICK_MESSAGE_METHOD, "Random", "Method for how messages should be picked from the lists", false, false, 0, false, 0, true)
		.bindTo(cvar_pick_message_method);

	// Message display method (Default, Notification)
	cvarManager->registerCvar(CVAR_DISPLAY_MESSAGE_METHOD, "Default", "Method for how messages will be displayed on the screen", false, false, 0, false, 0, true)
		.bindTo(cvar_display_message_method);

	// Comma separated messages
	cvarManager->registerCvar(CVAR_GOAL_MESSAGES, "Drink some water!,Check your posture!", "Comma separated messages to be displayed when a goal is scored", true, false, 0, false, 0, true)
		.addOnValueChanged([this](std::string oldVal, CVarWrapper cvar)
			{
				std::string messages = cvar.getStringValue();

				goalMessages.clear();

				FriendlyReminders::SplitString(messages, ',', goalMessages);
			}
	);

	cvarManager->registerCvar(CVAR_GAME_FINISHED_MESSAGES, "Do some push-ups!,Do some sit-ups!", "Comma separated messages to be displayed when a game is finished", true, false, 0, false, 0, true)
		.addOnValueChanged([this](std::string oldVal, CVarWrapper cvar)
			{
				std::string messages = cvar.getStringValue();

				gameFinishedMessages.clear();

				FriendlyReminders::SplitString(messages, ',', gameFinishedMessages);
			}
	);

	// Register rendering
	gameWrapper->RegisterDrawable(std::bind(&FriendlyReminders::Render, this, std::placeholders::_1));

	// Register hooks
	gameWrapper->HookEventPost("Function TAGame.Ball_TA.Explode", std::bind(&FriendlyReminders::HookGoalScored, this));

	gameWrapper->HookEventPost("Function GameEvent_Soccar_TA.ReplayPlayback.BeginState", std::bind(&FriendlyReminders::HookReplayBegin, this));
	gameWrapper->HookEventPost("Function GameEvent_Soccar_TA.ReplayPlayback.EndState", std::bind(&FriendlyReminders::HookReplayEnd, this));

	gameWrapper->HookEventPost("Function GameEvent_Soccar_TA.Countdown.BeginState", std::bind(&FriendlyReminders::HookCountdownBegin, this));
	gameWrapper->HookEventPost("Function TAGame.GameEvent_Soccar_TA.EventMatchEnded", std::bind(&FriendlyReminders::HookMatchEnded, this));

	gameWrapper->HookEventPost("Function TAGame.GFxShell_TA.LeaveMatch", std::bind(&FriendlyReminders::HookLeaveMatch, this));
}

// Nothing to unload
void FriendlyReminders::onUnload() {}

void FriendlyReminders::Render(CanvasWrapper canvas)
{
	Vector2 canvasSize = canvas.GetSize();

	float textScale = 5.0f * canvasSize.Y / 1080;

	Vector2F textPosition = { 0.5f, 0.37f };
	Vector2F textSize = canvas.GetStringSize(currentMessage, textScale, textScale);

	canvas.SetColor(LinearColor{ 255, 255, 255, 255 });
	canvas.SetPosition((textSize / -2) + textPosition * canvasSize);
	canvas.DrawString(currentMessage, textScale, textScale, true, false);
}

// Goal scored hook
void FriendlyReminders::HookGoalScored()
{
	// Check goal is not a replay
	if (isInReplay) return;
	// Check goal is not after-match replay
	if (!isInMatch) return;

	// Fire goal scored event
	FriendlyReminders::OnEvent(EventType::GoalScored);
}

// Replay began hook
void FriendlyReminders::HookReplayBegin()
{
	// Check replay hasn't already started
	if (!isInReplay)
	{
		isInReplay = true;
	}
}

// Replay ended hook
void FriendlyReminders::HookReplayEnd()
{
	// Check replay hasn't already finished
	if (isInReplay)
	{
		isInReplay = false;
	}
}

// Countdown began hook
void FriendlyReminders::HookCountdownBegin()
{
	// Check match hasn't already started
	if (!isInMatch)
	{
		isInMatch = true;
	}
}

// Match ended hook
void FriendlyReminders::HookMatchEnded()
{
	// Check match hasn't already finished
	if (isInMatch)
	{
		isInMatch = false;

		// Fire match ended event
		FriendlyReminders::OnEvent(EventType::GameFinished);
	}
}

// Leave match hook
void FriendlyReminders::HookLeaveMatch()
{
	// Check match hasn't already finished
	if (isInMatch)
	{
		isInMatch = false;

		// Fire match ended event
		FriendlyReminders::OnEvent(EventType::GameFinished);
	}
}

// Method to handle message events
void FriendlyReminders::OnEvent(EventType eventType)
{
	// Check event happened in online game
	if (!gameWrapper->IsInOnlineGame()) return;

	// Return if event type is disabled
	if (eventType == EventType::GoalScored && *cvar_show_goal_messages.get() == false) return;
	if (eventType == EventType::GameFinished && *cvar_show_game_finished_messages.get() == false) return;

	// Get next message for even type
	std::string message = FriendlyReminders::GetNextMessage(eventType);

	// Display message
	FriendlyReminders::DisplayMessage(message, 4);
}

// Method to get message for event type
std::string FriendlyReminders::GetNextMessage(EventType eventType)
{
	size_t goalMessagesLength = goalMessages.size();
	size_t gameFinishedMessagesLength = gameFinishedMessages.size();
	int maxIndex;
	int* messageIndex{};
	int displayMessageIndex;

	// Calculate maximum message index
	if (*cvar_combine_messages.get() == true)
	{
		maxIndex = goalMessagesLength + gameFinishedMessagesLength;

		messageIndex = &combinedMessageIndex;
	}
	else
	{
		if (eventType == EventType::GoalScored)
		{
			maxIndex = goalMessagesLength;

			messageIndex = &goalMessageIndex;
		}
		else// if (eventType == EventType::GameFinished)
		{
			maxIndex = gameFinishedMessagesLength;

			messageIndex = &gameFinishedMessageIndex;
		}
	}

	// Make sure there is a message
	if (maxIndex == 0) {
		return "";
	}

	// Determine next message index
	if (cvar_pick_message_method.get()->compare("Random") == 0)
	{
		displayMessageIndex = rand() % maxIndex;
	}
	else// if (cvar_pick_message_method.get()->compare("Indexed") == 0)
	{
		if (*messageIndex >= maxIndex)
		{
			*messageIndex = 0;
		}

		displayMessageIndex = *messageIndex;

		(*messageIndex)++;
	}

	// Get message at message index
	if (*cvar_combine_messages.get() == true)
	{
		if (displayMessageIndex < goalMessagesLength)
		{
			return goalMessages.at(displayMessageIndex);
		}
		else
		{
			return gameFinishedMessages.at(displayMessageIndex - goalMessagesLength);
		}
	}
	else
	{
		if (eventType == EventType::GoalScored)
		{
			return goalMessages.at(displayMessageIndex);
		}
		else// if (eventType == EventType::GameFinished)
		{
			return gameFinishedMessages.at(displayMessageIndex);
		}
	}
}

// Method to display message to user
void FriendlyReminders::DisplayMessage(std::string& message, int displayTime)
{
	currentMessageIndex++;

	// Check display method
	if (cvar_display_message_method.get()->compare("Default") == 0)
	{
		currentMessage = message;

		int thisMessageIndex = currentMessageIndex;

		// Clear message if it hasn't changed
		gameWrapper->SetTimeout(
			[this, thisMessageIndex](GameWrapper* gameWrapper)
			{
				if (currentMessageIndex != thisMessageIndex) return;

				currentMessage = "";
			},
			displayTime
		);
	}
	else// if (cvar_display_message_method.get()->compare("Notification") == 0)
	{
		// Get notifications enabled state
		bool notificiationsEnabled = cvarManager->getCvar("cl_notifications_enabled_beta").getBoolValue();

		// Enable notifications if currently disabled
		if (!notificiationsEnabled)
		{
			cvarManager->executeCommand("cl_notifications_enabled_beta 1");
		}

		// Display notification
		gameWrapper->Toast("Friendly Reminder", message, "default", displayTime);

		// Restore notifications enabled state
		if (!notificiationsEnabled)
		{
			cvarManager->executeCommand("cl_notifications_enabled_beta 0");
		}
	}
}

// String split method
void FriendlyReminders::SplitString(std::string& str, char delim, std::vector<std::string>& resultVector)
{
	std::istringstream iss(str);
	std::string subStr;

	while (std::getline(iss, subStr, delim))
	{
		resultVector.emplace_back(subStr);
	}
}