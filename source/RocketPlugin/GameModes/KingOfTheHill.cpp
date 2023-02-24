// GameModes/KingOfTheHill.cpp
// 1v1v1...
//

#include "KingOfTheHill.h"


/// <summary>Renders the available options for the game mode.</summary>
void KingOfTheHill::RenderOptions()
{
    ImGui::InputInt("Wins in a row to be king", &kothWins, 1, 1);
    if (kothWins <= 0) {
        kothWins = 1;
    }
}


/// <summary>Gets if the game mode is active.</summary>
/// <returns>Bool with if the game mode is active</returns>
bool KingOfTheHill::IsActive()
{
    return isActive;
}


/// <summary>Activates the game mode.</summary>
void KingOfTheHill::Activate(const bool active)
{
    if (active && !isActive) {
        HookEventWithCaller<ObjectWrapper>("Function TAGame.GFxHUD_TA.HandleStatTickerMessage",
            [this](const ObjectWrapper& caller, void* params, const std::string&) {
                statEvent(caller, params);
            });
    }
    else if (!active && isActive) {
        UnhookEvent("Function TAGame.GFxHUD_TA.HandleStatTickerMessage");
    }

    isActive = active;
}

struct StatEventStruct
{
    uintptr_t Receiver;
    uintptr_t Victim;
    uintptr_t StatEvent;
};

void KingOfTheHill::statEvent(ObjectWrapper obj, void* args) {
    auto stats = (StatEventStruct*)args;
    
    ServerWrapper sw = Outer()->GetGame();

    //auto victim = PriWrapper(tArgs->Victim);
    auto scorer = PriWrapper(stats->Receiver);
    auto statEvent = StatEventWrapper(stats->StatEvent);
    if (statEvent.IsNull() || scorer.IsNull() || sw.IsNull()) {
        return;
    }
    std::string eventString = statEvent.GetEventName();
    //rocketPlugin->cvarManager->log("event type: " + eventString + " by " + scorer.GetPlayerName().ToString());
    if (eventString != "Goal") {
        return;
    }
    auto scorerTeam = scorer.GetTeamNum();
    auto victimsTeam = (scorerTeam + 1) % 2;
    auto teams = sw.GetTeams();
    for (auto team : teams) {
        if (team.GetTeamNum() == scorerTeam) {
            auto goals = team.GetScore();
            if (goals >= kothWins) {
                // Declare King of the Hill
                //sw.EndGame(); // NO. It just freezes the game at the Goal
                //sw.EndRound(); // NO. Does nothing.
                //sw.SetMatchWinner(team); // NO. Doesn't do anything
                sw.SetGameTimeRemaining(.0f);
                return;
                //sw.SetGameWinner(team);
                //sw.SetMVP(team.GetMembers().Get(0));
            }
        }
        else if (team.GetTeamNum() == victimsTeam) {
            team.ResetScore();
        }
    }
    for (CarWrapper car : sw.GetCars()) {
        if (car.IsNull()) {
            continue;
        }
        if (car.GetTeamNum2() != victimsTeam) {
            continue;
        }
        auto pri = car.GetPRI();
        if (pri.IsNull()) {
            continue;
        }
        //rocketPlugin->cvarManager->log("player scored on: " + pri.GetPlayerName().ToString());
        if (!pri.GetbBot()) {
            pri.ServerSpectate();
        }
    }
}


/// <summary>Gets the game modes name.</summary>
/// <returns>The game modes name</returns>
std::string KingOfTheHill::GetGameModeName()
{
    return "KingOfTheHill";
}


/// <summary>Gets the game modes description.</summary>
/// <returns>The game modes description</returns>
std::string KingOfTheHill::GetGameModeDescription()
{
    return "KingOfTheHill - Keep your crown in a set of 1v1 matches.";
}
