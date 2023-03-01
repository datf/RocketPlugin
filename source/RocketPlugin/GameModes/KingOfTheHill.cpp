// GameModes/KingOfTheHill.cpp
// 1v1v1...
//

#include "KingOfTheHill.h"

std::list<int> gamersQueue;
std::list<int> gamersPlaying;

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

void KingOfTheHill::updateGamersQueue() {
    ServerWrapper sw = Outer()->GetGame();
    auto cars = sw.GetCars();
    auto players = sw.GetPlayers();
    auto playerCars = sw.GetPlayerCarCount();

    rocketPlugin->cvarManager->log("Updating gamers queue. Cars: " + std::to_string(cars.Count()) + ". players: " + std::to_string(players.Count()) + ". player cars: " + std::to_string(playerCars));
    for (auto car : cars) {
        auto pri = car.GetPRI();
        if (pri.IsNull()) continue;
        if (pri.GetbBot()) continue;
        auto pid = pri.GetPlayerID();
        if (std::find(gamersQueue.begin(), gamersQueue.end(), pid) == gamersQueue.end()
            && std::find(gamersPlaying.begin(), gamersPlaying.end(), pid) == gamersPlaying.end()) {
            gamersQueue.push_back(pid);
            rocketPlugin->cvarManager->log("New Queue PID: " + std::to_string(pid));
        }
    }
}
void KingOfTheHill::updateGamersPlaying() {
    ServerWrapper sw = Outer()->GetGame();
    auto cars = sw.GetCars();
    auto teams = sw.GetTeams();
    while (cars.Count() > 1 && gamersPlaying.size() < 2) {
        for (auto team : teams) {
            auto humans = team.GetMembers().Count() - team.GetNumBots();
            if (humans <= 0) {
                faceNextPlayer(team.GetTeamNum());
                break;
            }
        }
    }
}

/// <summary>Activates the game mode.</summary>
void KingOfTheHill::Activate(const bool active)
{
    if (active && !isActive) {
        HookEventWithCaller<ObjectWrapper>("Function TAGame.GFxHUD_TA.HandleStatTickerMessage",
            [this](const ObjectWrapper& caller, void* params, const std::string&) {
                statEvent(caller, params);
            });
        HookEvent("Function TAGame.GameEvent_TA.EventPlayerAdded",
            [this](const std::string& name) {
                updateGamersQueue();
                updateGamersPlaying();
            });
        HookEvent("Function TAGame.GameEvent_Soccar_TA.EventMatchEnded",
            [this](const std::string& name) {
                gamersQueue.clear();
                gamersPlaying.clear();
                rocketPlugin->cvarManager->log("EVENT: " + name);
            });
        //HookEvent("Function Function GameEvent_TA.Countdown.BeginState",
        //    [this](const std::string& name) {
        //        gamers.clear();
        //rocketPlugin->cvarManager->log("EVENT: " + name);
        //    });
        //HookEvent("Function TAGame.GameEvent_Soccar_TA.InitField",
        //    [this](const std::string& name) {
        //        gamers.clear();
        //rocketPlugin->cvarManager->log("EVENT: " + name);
        //    });
        //HookEvent("Function TAGame.GameEvent_TA.EventPlayerAdded",
        //    [this](const std::string& name) {
        //        gamers.clear();
        //rocketPlugin->cvarManager->log("EVENT: " + name);
        //    });

        
            
    }
    else if (!active && isActive) {
        UnhookEvent("Function TAGame.GFxHUD_TA.HandleStatTickerMessage");
        UnhookEvent("Function TAGame.GameEvent_TA.EventPlayerAdded");
        UnhookEvent("Function TAGame.GameEvent_Soccar_TA.EventMatchEnded");
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
                sw.SetGameTimeRemaining(.0f);
                //sw.SetGameWinner(team);
                //sw.SetMVP(team.GetMembers().Get(0));
                return;
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
        if (!pri.GetbBot() && gamersPlaying.size() >= 2) {
            // Now we have the player that got scored on. We have to reset all stats and send them to spectate.
            pri.SetMatchGoals(0);
            pri.SetMatchShots(0);
            pri.SetMatchSaves(0);
            pri.SetMatchScore(0);
            pri.ServerSpectate();
            auto pid = pri.GetPlayerID();
            gamersQueue.push_back(pid);
            gamersPlaying.remove(pid);
            rocketPlugin->cvarManager->log("Removed playing and added gamersQueue PID: " + std::to_string(pri.GetPlayerID()));
            faceNextPlayer(victimsTeam);
        }
    }
}

void KingOfTheHill::faceNextPlayer(int team) {
    ServerWrapper sw = Outer()->GetGame();
    auto cars = sw.GetCars();
    bool changed = false;
    int nextPlayer;
    while (!changed && gamersQueue.size() > 0) {
        nextPlayer = gamersQueue.front();
        for (auto c : cars) {
            if (c.IsNull()) continue;
            auto p = c.GetPRI();
            if (p.IsNull()) continue;
            if (p.GetPlayerID() != nextPlayer) continue;
            p.ServerChangeTeam(team);
            changed = true;
        }
        gamersQueue.pop_front();
    }
    if (changed) {
        gamersPlaying.push_back(nextPlayer);
        rocketPlugin->cvarManager->log("New gamersPlaying PID: " + std::to_string(nextPlayer));
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
