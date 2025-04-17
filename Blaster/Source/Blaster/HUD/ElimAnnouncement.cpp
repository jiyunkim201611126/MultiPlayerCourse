#include "ElimAnnouncement.h"

#include "ElimAnnouncementText.h"
#include "Components/VerticalBox.h"

void UElimAnnouncement::GenerateElimAnnouncementText(const FString& ElimAnnouncementText)
{
	if (AnnouncementTextClass)
	{
		UElimAnnouncementText* AnnouncementTextWidget = CreateWidget<UElimAnnouncementText>(this, AnnouncementTextClass);
		if (AnnouncementTextWidget)
		{
			AnnouncementTextWidget->SetAnnouncementText(FText::FromString(*ElimAnnouncementText));			
			AnnouncementBox->AddChildToVerticalBox(AnnouncementTextWidget);
		}
	}
}
