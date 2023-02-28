#pragma once
#include "GameModes/RocketGameMode.h"

class KingOfTheHill final : public RocketGameMode
{
public:
    KingOfTheHill() { typeIdx = std::make_unique<std::type_index>(typeid(*this)); }

    void RenderOptions() override;
    bool IsActive() override;
    void Activate(bool active) override;
    std::string GetGameModeName() override;
    std::string GetGameModeDescription() override;

private:
    void statEvent(ObjectWrapper caller, void* args);
    void faceNextPlayer(int team);
    void updateGamersQueue();
    void updateGamersPlaying();
    int kothWins = 7;
};
