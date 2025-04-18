#include "Chatting.h"

#include "Blaster/PlayerController/BlasterPlayerController.h"
#include "GameFramework/PlayerState.h"
#include "Components/TextBlock.h"
#include "Blueprint/WidgetTree.h"
#include "Components/EditableText.h"
#include "Components/ScrollBox.h"

void UChatting::NativeConstruct()
{
	Super::NativeConstruct();
	
	if (InputMessage)
	{
		// 엔터 및 Esc 입력에 대해 Focus 취소할 수 있도록 설정
		InputMessage->SetClearKeyboardFocusOnCommit(true);
		InputMessage->SetRevertTextOnEscape(true);

		// 엔터 입력 시 처리하는 함수 바인딩
		InputMessage->OnTextCommitted.AddDynamic(this, &ThisClass::ChatFocusOut);
	}
}

void UChatting::ChatFocusIn()
{
	if (InputMessage)
	{
		InputMessage->SetUserFocus(GetOwningPlayer());
		SetVisibility(ESlateVisibility::Visible);
		
		if (GetWorld()->GetTimerManager().IsTimerActive(ChatTimerHandle))
		{
			GetWorld()->GetTimerManager().ClearTimer(ChatTimerHandle);
		}
	}
}

void UChatting::ChatFocusOut(const FText& TextToChat, ETextCommit::Type CommitMethod)
{
	if (!InputMessage) return;

	ABlasterPlayerController* BlasterPlayerController = Cast<ABlasterPlayerController>(GetOwningPlayer());
	const FText PlayerNameText = FText::FromString(BlasterPlayerController->PlayerState->GetPlayerName());
	const FText CombinedText = FText::Format(FText::FromString(TEXT("{0}: {1}")), PlayerNameText, TextToChat);
	switch (CommitMethod)
	{
	case ETextCommit::OnEnter:
		if (TextToChat.IsEmpty()) break;
		BlasterPlayerController->ServerChat(CombinedText);
		BlasterPlayerController->SetInputGameOnly();
		if (MessageBox)
		{
			MessageBox->ScrollToEnd();
		}
		break;
	default:
		InputMessage->SetText(FText{});
		BlasterPlayerController->SetInputGameOnly();
		SetHideTimer();
		break;
	}
}

void UChatting::GenerateChat(const FText& TextToChat)
{
	if (!MessageBox) return;

	UTextBlock* NewChat = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass());
	if (NewChat)
	{
		NewChat->SetText(TextToChat);
		NewChat->SetAutoWrapText(true);
		MessageBox->AddChild(NewChat);
		if (FMath::IsNearlyEqual(MessageBox->GetScrollOffset(), MessageBox->GetScrollOffsetOfEnd(), 1.0f))
		{
			MessageBox->ScrollToEnd();
		}
		SetVisibility(ESlateVisibility::Visible);
		SetHideTimer();
	}
}

void UChatting::SetHideTimer()
{
	GetWorld()->GetTimerManager().SetTimer(
		ChatTimerHandle,
		[this]()
		{
			SetVisibility(ESlateVisibility::Hidden);
		},
		MessagesHiddenTimer,
		false
		);
}
