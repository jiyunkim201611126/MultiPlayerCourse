#include "Announcement.h"

#include "Components/TextBlock.h"

void UAnnouncement::UpdateWarmupTimeText(const FString& InString) const
{
	WarmupTimeText->SetText(FText::FromString(InString));
}

void UAnnouncement::UpdateAnnouncementText(const FString& InString) const
{
	AnnouncementText->SetText(FText::FromString(InString));
}

void UAnnouncement::UpdateInfoText(const FString& InString) const
{
	InfoText->SetText(FText::FromString(InString));
}
