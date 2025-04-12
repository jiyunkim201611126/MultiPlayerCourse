#include "ReturnToMainMenu.h"

#include "GameFramework/PlayerController.h"
#include "Components/Button.h"
#include "MultiplayerSessions/Public/MenuSystem/MultiplayerSessionsSubsystem.h"
#include "GameFramework/GameModeBase.h"
#include "Blaster/Character/BlasterCharacter.h"

void UReturnToMainMenu::MenuSetup()
{
	AddToViewport();
	SetVisibility(ESlateVisibility::Visible);
	SetIsFocusable(true);
	
	if (ReturnButton && !ReturnButton->OnClicked.IsBound())
	{
		ReturnButton->OnClicked.AddDynamic(this, &ThisClass::ReturnButtonClicked);
	}

	if (UGameInstance* GameInstance = GetGameInstance())
	{
		MultiplayerSessionsSubsystem = GameInstance->GetSubsystem<UMultiplayerSessionsSubsystem>();
		if (MultiplayerSessionsSubsystem && !MultiplayerSessionsSubsystem->MultiPlayerOnDestroySessionComplete.IsBound())
		{
			MultiplayerSessionsSubsystem->MultiPlayerOnDestroySessionComplete.AddDynamic(this, &ThisClass::OnDestroySession);
		}
	}
}

void UReturnToMainMenu::MenuTearDown()
{
	RemoveFromParent();
	
	if (ReturnButton && ReturnButton->OnClicked.IsBound())
	{
		ReturnButton->OnClicked.RemoveDynamic(this, &ThisClass::ReturnButtonClicked);
	}
	
	if (MultiplayerSessionsSubsystem && MultiplayerSessionsSubsystem->MultiPlayerOnDestroySessionComplete.IsBound())
	{
		MultiplayerSessionsSubsystem->MultiPlayerOnDestroySessionComplete.RemoveDynamic(this, &ThisClass::OnDestroySession);
	}
}

FReply UReturnToMainMenu::NativeOnMouseButtonDown(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent)
{
	Super::NativeOnMouseButtonDown(InGeometry, InMouseEvent);
	return FReply::Handled();
}

void UReturnToMainMenu::ReturnButtonClicked()
{
	ReturnButton->SetIsEnabled(false);

	UWorld* World = GetWorld();
	if (World)
	{
		APlayerController* FirstPlayerController = World->GetFirstPlayerController();
		if (FirstPlayerController)
		{
			ABlasterCharacter* BlasterCharacter = Cast<ABlasterCharacter>(FirstPlayerController->GetPawn());
			if (BlasterCharacter)
			{
				BlasterCharacter->OnLeftGame.AddDynamic(this, &ThisClass::OnPlayerLeftGame);
				BlasterCharacter->ServerLeaveGame();
			}
			else
			{
				ReturnButton->SetIsEnabled(true);
			}
		}
	}
}

void UReturnToMainMenu::OnPlayerLeftGame()
{
	if (MultiplayerSessionsSubsystem)
	{
		MultiplayerSessionsSubsystem->DestroySession();
	}
}

void UReturnToMainMenu::OnDestroySession(bool bWasSuccessful)
{
	if (!bWasSuccessful)
	{
		ReturnButton->SetIsEnabled(true);
		return;
	}
	
	if (UWorld* World = GetWorld())
	{
		if (AGameModeBase* GameModeBase = World->GetAuthGameMode<AGameModeBase>())
		{
			GameModeBase->ReturnToMainMenuHost();
		}
		else
		{
			PlayerController = PlayerController == nullptr ? World->GetFirstPlayerController() : PlayerController;
			if (PlayerController)
			{
				PlayerController->ClientReturnToMainMenuWithTextReason(FText());
			}
		}
	}
}
