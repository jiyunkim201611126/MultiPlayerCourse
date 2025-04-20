#include "TeamsGameMode.h"
#include "Blaster/GameState/BlasterGameState.h"
#include "Blaster/PlayerState/BlasterPlayerState.h"
#include "Blaster/PlayerController/BlasterPlayerController.h"

ATeamsGameMode::ATeamsGameMode()
{
	bTeamsMatch = true;
}

void ATeamsGameMode::PostLogin(APlayerController* NewPlayer)
{
	Super::PostLogin(NewPlayer);

	ABlasterGameState* BlasterGameState = GetGameState<ABlasterGameState>();
	if (BlasterGameState)
	{
		ABlasterPlayerState* BlasterPlayerState = NewPlayer->GetPlayerState<ABlasterPlayerState>();
		if (BlasterPlayerState && BlasterPlayerState->GetTeam() == ETeam::ET_NoTeam)
		{
			if (BlasterGameState->BlueTeam.Num() >= BlasterGameState->RedTeam.Num())
			{
				BlasterGameState->RedTeam.AddUnique(BlasterPlayerState);
				BlasterPlayerState->SetTeam(ETeam::ET_RedTeam);
			}
			else
			{
				BlasterGameState->BlueTeam.AddUnique(BlasterPlayerState);
				BlasterPlayerState->SetTeam(ETeam::ET_BlueTeam);
			}
			ABlasterPlayerController* BlasterPlayerController = Cast<ABlasterPlayerController>(NewPlayer);
			if (BlasterPlayerController)
			{
				BlasterPlayerController->bShowTeamScores = bTeamsMatch;
			}
		}
	}
}

void ATeamsGameMode::Logout(AController* Exiting)
{
	Super::Logout(Exiting);

	ABlasterGameState* BlasterGameState = GetGameState<ABlasterGameState>();
	ABlasterPlayerState* BlasterPlayerState = Exiting->GetPlayerState<ABlasterPlayerState>();
	if (BlasterGameState && BlasterPlayerState)
	{
		if (BlasterGameState->RedTeam.Contains(BlasterPlayerState))
		{
			BlasterGameState->RedTeam.Remove(BlasterPlayerState);
		}
		if (BlasterGameState->BlueTeam.Contains(BlasterPlayerState))
		{
			BlasterGameState->BlueTeam.Remove(BlasterPlayerState);
		}
	}
}

void ATeamsGameMode::PlayerEliminated(ABlasterCharacter* EliminatedCharacter,
	ABlasterPlayerController* VictimController, ABlasterPlayerController* AttackerController)
{
	Super::PlayerEliminated(EliminatedCharacter, VictimController, AttackerController);
	
	ABlasterGameState* BlasterGameState = GetGameState<ABlasterGameState>();
	ABlasterPlayerState* AttackerPlayerState = AttackerController ? Cast<ABlasterPlayerState>(AttackerController->PlayerState) : nullptr;
	ABlasterPlayerState* VictimPlayerState = VictimController ? Cast<ABlasterPlayerState>(VictimController->PlayerState) : nullptr;

	// 팀킬인 경우 바로 리턴
	if (AttackerPlayerState->GetTeam() == VictimPlayerState->GetTeam()) return;

	// 팀 점수 획득
	if (BlasterGameState && AttackerPlayerState)
	{
		if (AttackerPlayerState->GetTeam() == ETeam::ET_RedTeam)
		{
			BlasterGameState->RedTeamScores();
		}
		if (AttackerPlayerState->GetTeam() == ETeam::ET_BlueTeam)
		{
			BlasterGameState->BlueTeamScores();
		}
	}
}

void ATeamsGameMode::HandleMatchHasStarted()
{
	Super::HandleMatchHasStarted();

	ABlasterGameState* BlasterGameState = GetGameState<ABlasterGameState>();
	if (BlasterGameState)
	{
		for (auto PlayerState : BlasterGameState->PlayerArray)
		{
			ABlasterPlayerState* BlasterPlayerState = Cast<ABlasterPlayerState>(PlayerState);
			if (BlasterPlayerState && BlasterPlayerState->GetTeam() == ETeam::ET_NoTeam)
			{
				if (BlasterGameState->BlueTeam.Num() >= BlasterGameState->RedTeam.Num())
				{
					BlasterGameState->RedTeam.AddUnique(BlasterPlayerState);
					BlasterPlayerState->SetTeam(ETeam::ET_RedTeam);
				}
				else
				{
					BlasterGameState->BlueTeam.AddUnique(BlasterPlayerState);
					BlasterPlayerState->SetTeam(ETeam::ET_BlueTeam);
				}
			}
		}
	}
}
