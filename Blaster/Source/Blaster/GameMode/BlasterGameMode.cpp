#include "BlasterGameMode.h"
#include "Blaster/Character/BlasterCharacter.h"
#include "Blaster/PlayerController/BlasterPlayerController.h"
#include "Kismet/GameplayStatics.h"
#include "GameFramework/PlayerStart.h"
#include "Blaster/PlayerState/BlasterPlayerState.h"
#include "Blaster/GameState/BlasterGameState.h"

namespace MatchState
{
	const FName Cooldown = FName("Cooldown");
}

ABlasterGameMode::ABlasterGameMode()
{
	// 게임을 바로 시작하지 않고 대기 시간을 갖도록 함
	bDelayedStart = true;
}

bool ABlasterGameMode::CheckTeammate(AController* InstigatorController, AController* DamagedController)
{
	if (!bTeamsMatch) return false;
	
	const ABlasterPlayerState* InstigatorState = InstigatorController->GetPlayerState<ABlasterPlayerState>();
	const ABlasterPlayerState* DamagedState = DamagedController->GetPlayerState<ABlasterPlayerState>();
	
	if (InstigatorState && DamagedState)
	{
		if (InstigatorState->GetTeam() == DamagedState->GetTeam())
		{
			return true;
		}
	}

	return false;
}

void ABlasterGameMode::BeginPlay()
{
	Super::BeginPlay();

	// 레벨 시작 시간 기록
	LevelStartingTime = GetWorld()->GetTimeSeconds();
}

void ABlasterGameMode::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	// 게임 시작 대기 상태면
	if (MatchState == MatchState::WaitingToStart)
	{
		// WarmupTime만큼 시간 센 뒤 매치 시작
		CountdownTime = WarmupTime - GetWorld()->GetTimeSeconds() + LevelStartingTime;
		if (CountdownTime <= 0.f)
		{
			StartMatch();
		}
	}
	// 게임 플레이 중 상태면
	else if (MatchState == MatchState::InProgress)
	{
		// MatchTime만큼 시간 센 뒤 게임 상태를 Cooldown으로 변경
		CountdownTime = WarmupTime + MatchTime - GetWorld()->GetTimeSeconds() + LevelStartingTime;
		if (CountdownTime <= 0.f)
		{
			SetMatchState(MatchState::Cooldown);
		}
	}
	else if (MatchState == MatchState::Cooldown)
	{
		CountdownTime = CooldownTime + WarmupTime + MatchTime - GetWorld()->GetTimeSeconds() + LevelStartingTime;
		if (CountdownTime <= 0.f)
		{
			RestartGame();
		}
	}
}

void ABlasterGameMode::OnMatchStateSet()
{
	Super::OnMatchStateSet();

	// 모든 플레이어에게 MatchState의 변경 사항에 대해 알림
	for (FConstPlayerControllerIterator It = GetWorld()->GetPlayerControllerIterator(); It; ++It)
	{
		if (ABlasterPlayerController* BlasterPlayer = Cast<ABlasterPlayerController>(It->Get()))
		{
			BlasterPlayer->OnMatchStateSet(MatchState, bTeamsMatch);
		}
	}
}

void ABlasterGameMode::PlayerEliminated(ABlasterCharacter* EliminatedCharacter,
                                        ABlasterPlayerController* VictimController,
                                        ABlasterPlayerController* AttackerController)
{
	if (AttackerController == nullptr || AttackerController->PlayerState == nullptr) return;
	if (VictimController == nullptr || VictimController->PlayerState == nullptr) return;
	ABlasterPlayerState* AttackerPlayerState = AttackerController ? Cast<ABlasterPlayerState>(AttackerController->PlayerState) : nullptr;
	ABlasterPlayerState* VictimPlayerState = VictimController ? Cast<ABlasterPlayerState>(VictimController->PlayerState) : nullptr;

	// 캐릭터 사망
	if (EliminatedCharacter)
	{
		EliminatedCharacter->Elim(false);
	}

	// 킬로그 전파
	for (FConstPlayerControllerIterator It = GetWorld()->GetPlayerControllerIterator(); It; ++It)
	{
		ABlasterPlayerController* BlasterPlayer = Cast<ABlasterPlayerController>(It->Get());
		if (BlasterPlayer && AttackerPlayerState && VictimPlayerState)
		{
			BlasterPlayer->BroadcastElim(AttackerPlayerState, VictimPlayerState);
		}
	}

	// 팀킬한 경우 Elim 알림만 호출하고 리턴
	if (bTeamsMatch && AttackerPlayerState->GetTeam() == VictimPlayerState->GetTeam()) return;

	// 점수 관련 처리들
	ABlasterGameState* BlasterGameState = GetGameState<ABlasterGameState>();
	if (AttackerPlayerState && AttackerPlayerState != VictimPlayerState && BlasterGameState)
	{
		TArray<ABlasterPlayerState*> PlayerCurrentlyInTheLead;

		for (auto LeadPlayer : BlasterGameState->TopScoringPlayers)
		{
			PlayerCurrentlyInTheLead.Add(LeadPlayer);
		}
		
		// 점수 올려주고 TopScore 갱신 함수 호출
		AttackerPlayerState->AddToScore(1.f);
		BlasterGameState->UpdateTopScore(AttackerPlayerState);
		if (BlasterGameState->TopScoringPlayers.Contains(AttackerPlayerState))
		{
			ABlasterCharacter* Leader = Cast<ABlasterCharacter>(AttackerPlayerState->GetPawn());
			if (Leader)
			{
				Leader->MulticastGainedTheLead();
			}
		}

		for (int32 i = 0; i < PlayerCurrentlyInTheLead.Num(); i++)
		{
			if (!BlasterGameState->TopScoringPlayers.Contains(PlayerCurrentlyInTheLead[i]))
			{
				ABlasterCharacter* Loser = Cast<ABlasterCharacter>(PlayerCurrentlyInTheLead[i]->GetPawn());
				if (Loser)
				{
					Loser->MulticastLostTheLead();
				}
			}
		}
	}

	if (VictimPlayerState)
	{
		VictimPlayerState->AddToDefeats(1);
	}
}

void ABlasterGameMode::RequestRespawn(ACharacter* ElimmedCharacter, AController* ElimmedController)
{
	if (ElimmedCharacter)
	{
		ElimmedCharacter->Reset();
		ElimmedCharacter->Destroy();
	}
	
	if (ElimmedController && MatchState == MatchState::InProgress)
	{
		TArray<AActor*> PlayerStarts;
		UGameplayStatics::GetAllActorsOfClass(this, APlayerStart::StaticClass(), PlayerStarts);

		TArray<FVector> PlayerLocations;
		for (FConstPlayerControllerIterator It = GetWorld()->GetPlayerControllerIterator(); It; ++It)
		{
			APlayerController* PlayerController = It->Get();
			if (PlayerController && PlayerController != ElimmedController && PlayerController->GetPawn())
			{
				PlayerLocations.Add(PlayerController->GetFocalLocation());
			}
		}

		AActor* PlayerStartToSpawn = nullptr;
		float MaxMinDistance = -1.0f;
		for (AActor* PlayerStart : PlayerStarts)
		{
			FVector PlayerStartLocation = PlayerStart->GetActorLocation();

			// 현재 PlayerStart와 모든 플레이어 간의 최소 거리 계산
			float MinDistance = FLT_MAX;
			for (const FVector& PlayerLocation : PlayerLocations)
			{
				float Distance = FVector::Distance(PlayerStartLocation, PlayerLocation);
				if (Distance < MinDistance)
				{
					MinDistance = Distance;
				}
			}

			// 최소 거리가 가장 큰 PlayerStart를 선택
			if (MinDistance > MaxMinDistance)
			{
				MaxMinDistance = MinDistance;
				PlayerStartToSpawn = PlayerStart;
			}
		}
		
		RestartPlayerAtPlayerStart(ElimmedController, PlayerStartToSpawn);
	}
}

void ABlasterGameMode::PlayerLeftGame(ABlasterPlayerState* PlayerLeaving)
{
	if (PlayerLeaving == nullptr) return;

	ABlasterGameState* BlasterGameState = GetGameState<ABlasterGameState>();
	if (BlasterGameState && BlasterGameState->TopScoringPlayers.Contains(PlayerLeaving))
	{
		BlasterGameState->TopScoringPlayers.Remove(PlayerLeaving);
	}

	ABlasterCharacter* CharacterLeaving = Cast<ABlasterCharacter>(PlayerLeaving->GetPawn());
	if (CharacterLeaving)
	{
		CharacterLeaving->Elim(true);
	}
}
