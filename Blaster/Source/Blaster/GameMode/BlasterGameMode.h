#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameMode.h"
#include "BlasterGameMode.generated.h"

namespace  MatchState
{
	extern BLASTER_API const FName Cooldown; // Match duration has been reached. Display winner and begin cooldown timer.
}

UCLASS()
class BLASTER_API ABlasterGameMode : public AGameMode
{
	GENERATED_BODY()

public:
	ABlasterGameMode();
	virtual void Tick(float DeltaTime) override;
	virtual void PlayerEliminated(class ABlasterCharacter* EliminatedCharacter, class ABlasterPlayerController* VictimController, ABlasterPlayerController* AttackerController);
	virtual void RequestRespawn(ACharacter* ElimmedCharacter, AController* ElimmedController);
	void PlayerLeftGame(class ABlasterPlayerState* PlayerLeaving);

	// 레벨이 시작된 시간 (게임 시작 시간이랑 다름)
	float LevelStartingTime = 0.f;
	
	// 게임 시작 대기 시간
	UPROPERTY(EditDefaultsOnly)
	float WarmupTime = 10.f;

	// 매치 시간
	UPROPERTY(EditDefaultsOnly)
	float MatchTime = 120.f;

	UPROPERTY(EditDefaultsOnly)
	float CooldownTime = 10.f;
	
	bool CheckTeammate(AController* InstigatorController, AController* DamagedController);
	
	bool bTeamsMatch = false;

protected:
	virtual void BeginPlay() override;

	// MatchState에 변경 사항이 있는 경우 호출되는 함수
	virtual void OnMatchStateSet() override;

private:
	// 위젯에 표시할 카운트다운
	float CountdownTime = 0.f;

public:
	FORCEINLINE float GetCountdownTime() const { return CountdownTime; }
};
