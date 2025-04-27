#pragma once

#include "CoreMinimal.h"
#include "BlasterGameMode.h"
#include "TeamsGameMode.generated.h"

UCLASS()
class BLASTER_API ATeamsGameMode : public ABlasterGameMode
{
	GENERATED_BODY()

public:
	ATeamsGameMode();
	virtual void PostLogin(APlayerController* NewPlayer) override;
	virtual void Logout(AController* Exiting) override;
	virtual void PlayerEliminated(ABlasterCharacter* EliminatedCharacter, ABlasterPlayerController* VictimController, ABlasterPlayerController* AttackerController) override;

	// 팀 별 스폰 위치가 따로 있으므로 override해 사용
	virtual void RequestRespawn(ACharacter* ElimmedCharacter, AController* ElimmedController) override;

protected:
	virtual void HandleMatchHasStarted() override;
};
