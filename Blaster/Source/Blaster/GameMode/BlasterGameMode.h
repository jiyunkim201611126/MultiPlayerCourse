#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameMode.h"
#include "BlasterGameMode.generated.h"

UCLASS()
class BLASTER_API ABlasterGameMode : public AGameMode
{
	GENERATED_BODY()

public:
	ABlasterGameMode();
	virtual void Tick(float DeltaTime) override;
	virtual void PlayerEliminated(class ABlasterCharacter* EliminatedCharacter, class ABlasterPlayerController* VictimController, ABlasterPlayerController* AttackerController);
	virtual void RequestRespawn(ACharacter* ElimmedCharacter, AController* ElimmedController);

	// 게임 시작 대기 시간
	UPROPERTY(EditDefaultsOnly)
	float WarmupTime = 10.f;

	// 레벨이 시작된 시간 (게임 시작 시간이랑 다름)
	float LevelStartingTime = 0.f;

protected:
	virtual void BeginPlay() override;

private:
	// 게임 시작 카운트다운
	float CountdownTime = 0.f;
};
