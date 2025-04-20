#include "BlasterGameState.h"

#include "Net/UnrealNetwork.h"
#include "Blaster/PlayerState/BlasterPlayerState.h"
#include "Blaster/PlayerController/BlasterPlayerController.h"

void ABlasterGameState::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(ABlasterGameState, TopScoringPlayers);
	DOREPLIFETIME(ABlasterGameState, RedTeamScore);
	DOREPLIFETIME(ABlasterGameState, BlueTeamScore);
}

void ABlasterGameState::UpdateTopScore(ABlasterPlayerState* ScoringPlayer)
{
	if (TopScoringPlayers.Num() == 0)
	{
		TopScoringPlayers.Add(ScoringPlayer);
		TopScore = ScoringPlayer->GetScore();
	}
	else if (ScoringPlayer->GetScore() == TopScore)
	{
		TopScoringPlayers.AddUnique(ScoringPlayer);
	}
	else if (ScoringPlayer->GetScore() > TopScore)
	{
		TopScoringPlayers.Empty();
		TopScoringPlayers.AddUnique(ScoringPlayer);
		TopScore = ScoringPlayer->GetScore();
	}
}

void ABlasterGameState::RedTeamScores()
{
	++RedTeamScore;

	ABlasterPlayerController* BlasterPlayerController = Cast<ABlasterPlayerController>(GetWorld()->GetFirstPlayerController());
	if (BlasterPlayerController)
	{
		BlasterPlayerController->SetHUDRedTeamScore(RedTeamScore);
	}
}

void ABlasterGameState::BlueTeamScores()
{
	++BlueTeamScore;

	ABlasterPlayerController* BlasterPlayerController = Cast<ABlasterPlayerController>(GetWorld()->GetFirstPlayerController());
	if (BlasterPlayerController)
	{
		BlasterPlayerController->SetHUDBlueTeamScore(BlueTeamScore);
	}
}

void ABlasterGameState::OnRep_RedTeamScore()
{
	ABlasterPlayerController* BlasterPlayerController = Cast<ABlasterPlayerController>(GetWorld()->GetFirstPlayerController());
	if (BlasterPlayerController)
	{
		BlasterPlayerController->SetHUDRedTeamScore(RedTeamScore);
	}
}

void ABlasterGameState::OnRep_BlueTeamScore()
{
	ABlasterPlayerController* BlasterPlayerController = Cast<ABlasterPlayerController>(GetWorld()->GetFirstPlayerController());
	if (BlasterPlayerController)
	{
		BlasterPlayerController->SetHUDBlueTeamScore(BlueTeamScore);
	}
}
