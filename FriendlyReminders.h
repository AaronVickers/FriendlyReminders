#pragma once
#pragma comment(lib, "pluginsdk.lib")
#include "bakkesmod/plugin/bakkesmodplugin.h"

enum class EventType
{
	GoalScored,
	GameFinished,
	//LeaveMatch
};

class FriendlyReminders : public BakkesMod::Plugin::BakkesModPlugin
{
public:
	// BakkesModPlugin methods
	virtual void onLoad();
	virtual void onUnload();

private:
	// CVARs
	std::shared_ptr<bool> cvar_show_goal_messages;
	std::shared_ptr<bool> cvar_show_game_finished_messages;
	std::shared_ptr<bool> cvar_combine_messages;
	std::shared_ptr<std::string> cvar_pick_message_method;
	std::shared_ptr<std::string> cvar_display_message_method;
	std::shared_ptr<float> cvar_message_scale;
	std::shared_ptr<float> cvar_message_position_x;
	std::shared_ptr<float> cvar_message_position_y;
	std::shared_ptr<float> cvar_message_anchor_x;
	std::shared_ptr<float> cvar_message_anchor_y;
	//std::shared_ptr<std::string> cvar_goal_messages;
	//std::shared_ptr<std::string> cvar_game_finished_messages;

	// CVAR post-process results
	std::vector<std::string> goalMessages = { "Drink some water!", "Check your posture!", };
	std::vector<std::string> gameFinishedMessages = { "Do some push-ups!", "Do some sit-ups!", };

	// Plugin flags
	bool isInMatch = false;
	bool isInReplay = false;
	int goalMessageIndex = 0;
	int gameFinishedMessageIndex = 0;
	int combinedMessageIndex = 0;
	std::string currentMessage = "";
	int currentMessageIndex = 0;

	// Interface rendering
	void Render(CanvasWrapper);

	// Hooks
	void HookGoalScored();
	void HookReplayBegin();
	void HookReplayEnd();
	void HookCountdownBegin();
	void HookMatchEnded();
	void HookLeaveMatch();

	// Plugin methods
	void OnEvent(EventType);
	std::string GetNextMessage(EventType);
	void DisplayMessage(std::string&, int);

	// Utility methods
	void SplitString(std::string&, char, std::vector<std::string>&);
};