#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "ElimAnnouncementText.generated.h"

UCLASS()
class BLASTER_API UElimAnnouncementText : public UUserWidget
{
	GENERATED_BODY()

public:
	void SetAnnouncementText(const FText& Text);

private:
	UPROPERTY(meta = (BindWidget), meta = (AllowPrivateAccess = "true"))
	TObjectPtr<class UTextBlock> AnnouncementText;

	UPROPERTY(EditAnywhere)
	float AnnouncementTime = 4.f;
};
