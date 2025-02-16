#include "BlasterGameMode.h"
#include "Blaster/Character/BlasterCharacter.h"
#include "Blaster/PlayerController/BlasterPlayerController.h"
#include "Kismet/GameplayStatics.h"
#include "GameFramework/PlayerStart.h"
#include "Blaster/PlayerState/BlasterPlayerState.h"

void ABlasterGameMode::PlayerEliminated(ABlasterCharacter* EliminatedCharacter,
                                        ABlasterPlayerController* VictimController,
                                        ABlasterPlayerController* AttackerController)
{
	ABlasterPlayerState* AttackerPlayerState = AttackerController ? Cast<ABlasterPlayerState>(AttackerController->PlayerState) : nullptr;
	ABlasterPlayerState* VictimPlayerState = VictimController ? Cast<ABlasterPlayerState>(VictimController->PlayerState) : nullptr;

	if (AttackerPlayerState && AttackerPlayerState != VictimPlayerState)
	{
		AttackerPlayerState->AddToScore(1.f);
	}

	if (VictimPlayerState)
	{
		VictimPlayerState->AddToDefeats(1);
	}
	
	if (EliminatedCharacter)
	{
		EliminatedCharacter->Elim();
	}
}

void ABlasterGameMode::RequestRespawn(ACharacter* ElimmedCharacter, AController* ElimmedController)
{
	if (ElimmedCharacter)
	{
		ElimmedCharacter->Reset();
		ElimmedCharacter->Destroy();
	}
	if (ElimmedController)
	{
		TArray<AActor*> PlayerStarts;
		UGameplayStatics::GetAllActorsOfClass(this, APlayerStart::StaticClass(), PlayerStarts);

		TArray<FVector> PlayerLocations;
		for (FConstPlayerControllerIterator It = GetWorld()->GetPlayerControllerIterator(); It; ++It)
		{
			APlayerController* PlayerController = It->Get();
			if (PlayerController && PlayerController != ElimmedController && PlayerController->GetPawn())
			{
				PlayerLocations.Add(PlayerController->GetPawn()->GetActorLocation());
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
