#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameState.h"
#include "BlasterGameState.generated.h"

UCLASS()
class BLASTER_API ABlasterGameState : public AGameState
{
	GENERATED_BODY()

public:
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
	void UpdateTopScore(class ABlasterPlayerState* ScoringPlayer);
	
	UPROPERTY(Replicated)
	TArray<ABlasterPlayerState*> TopScoringPlayers;

	/**
	 * Teams
	 */

	void RedTeamScores();
	void BlueTeamScores();

	UPROPERTY()
	TArray<ABlasterPlayerState*> RedTeam;
	UPROPERTY()
	TArray<ABlasterPlayerState*> BlueTeam;

	UPROPERTY(ReplicatedUsing = OnRep_RedTeamScore)
	float RedTeamScore = 0.f;
	UPROPERTY(ReplicatedUsing = OnRep_BlueTeamScore)
	float BlueTeamScore = 0.f;
	
private:
	UFUNCTION()
	void OnRep_RedTeamScore();
	UFUNCTION()
	void OnRep_BlueTeamScore();
	
	float TopScore = 0.f;
};
