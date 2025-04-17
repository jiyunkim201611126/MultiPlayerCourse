#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "ElimAnnouncement.generated.h"

UCLASS()
class BLASTER_API UElimAnnouncement : public UUserWidget
{
	GENERATED_BODY()

public:
	void GenerateElimAnnouncementText(const FString& ElimAnnouncementText);
	
	UPROPERTY(meta=(BindWidget))
	TObjectPtr<class UVerticalBox> AnnouncementBox;

	UPROPERTY(EditAnywhere)
	TSubclassOf<class UElimAnnouncementText> AnnouncementTextClass;
};
