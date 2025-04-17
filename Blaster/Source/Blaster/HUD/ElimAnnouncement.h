#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "ElimAnnouncement.generated.h"

UCLASS()
class BLASTER_API UElimAnnouncement : public UUserWidget
{
	GENERATED_BODY()

public:
	void SetElimAnnouncementText(const FString& AttackerName, const FString& VictimName);
	
	UPROPERTY(meta=(BindWidget))
	TObjectPtr<class UHorizontalBox> AnnouncementBox;

	UPROPERTY(meta=(BindWidget))
	TObjectPtr<class UTextBlock> AnnouncementText;
};
