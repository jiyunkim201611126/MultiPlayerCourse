#include "ElimAnnouncementText.h"

#include "Components/TextBlock.h"

void UElimAnnouncementText::SetAnnouncementText(const FText& Text)
{
	if (AnnouncementText == nullptr) return;
	
	AnnouncementText->SetText(Text);
	AnnouncementText->SetJustification(ETextJustify::Right);
		
	FTimerHandle AnnouncementTimer;
	FTimerDelegate AnnouncementDelegate;
	AnnouncementDelegate.BindLambda([this]()
	{
		RemoveFromParent();
	});

	GetWorld()->GetTimerManager().SetTimer(
		AnnouncementTimer,
		AnnouncementDelegate,
		AnnouncementTime,
		false
		);
}
