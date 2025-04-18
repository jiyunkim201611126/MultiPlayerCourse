#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Chatting.generated.h"

UCLASS()
class BLASTER_API UChatting : public UUserWidget
{
	GENERATED_BODY()
	
	UPROPERTY(meta = (BindWidget), meta = (AllowPrivateAccess = "true"))
	TObjectPtr<class UScrollBox> MessageBox;

	UPROPERTY(meta = (BindWidget), meta = (AllowPrivateAccess = "true"))
	TObjectPtr<class UEditableText> InputMessage;

public:
	virtual void NativeConstruct() override;
	
	void ChatFocusIn();
	UFUNCTION()
	void ChatFocusOut(const FText& TextToChat, ETextCommit::Type CommitMethod);

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	float MessagesHiddenTimer = 15.f;
	
	void GenerateChat(const FText& TextToChat);

	void SetHideTimer();
	FTimerHandle ChatTimerHandle;
};
