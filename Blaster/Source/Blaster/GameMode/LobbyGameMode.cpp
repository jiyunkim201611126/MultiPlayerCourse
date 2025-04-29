#include "LobbyGameMode.h"
#include "GameFramework/GameStateBase.h"
#include "MultiplayerSessions/Public//MenuSystem/MultiplayerSessionsSubsystem.h"

void ALobbyGameMode::StartGame()
{
	UWorld* World = GetWorld();
	UGameInstance* GameInstance = GetGameInstance();
	if (GameInstance)
	{
		UMultiplayerSessionsSubsystem* Subsystem = GameInstance->GetSubsystem<UMultiplayerSessionsSubsystem>();
		check(Subsystem);

		if (World)
		{
			bUseSeamlessTravel = true;

			FString MatchType = Subsystem->DesiredMatchType;
			if (MatchType == "DeathMatch")
			{
				World->ServerTravel(FString("/Game/Maps/DeathMatch?listen"));
			}
			else if (MatchType == "TeamDeathMatch")
			{
				World->ServerTravel(FString("/Game/Maps/TeamDeathMatch?listen"));
			}
			else if (MatchType == "CaptureTheFlag")
			{
				World->ServerTravel(FString("/Game/Maps/CaptureTheFlag?listen"));
			}
		}
	}
}
