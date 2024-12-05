#include "OverheadWidget.h"
#include "Components/TextBlock.h"

void UOverheadWidget::SetPlayerName(FString PlayerName)
{
	if (DisplayText)
	{
		DisplayText->SetText(FText::FromString(PlayerName));
	}
}

void UOverheadWidget::NativeDestruct()
{
	RemoveFromParent();
	
	Super::NativeDestruct();
}
