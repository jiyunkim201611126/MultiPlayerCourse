#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Components/TimelineComponent.h"
#include "CharacterOverlay.generated.h"

UCLASS()
class BLASTER_API UCharacterOverlay : public UUserWidget
{
	GENERATED_BODY()

public:
	UPROPERTY(meta = (BindWidget))
	class UProgressBar* HealthBar;

	UPROPERTY(meta = (BindWidget))
	class UTextBlock* HealthTextBlock;
	
	UPROPERTY(meta = (BindWidget))
	class UProgressBar* ShieldBar;

	UPROPERTY(meta = (BindWidget))
	class UTextBlock* ShieldTextBlock;
	
	UPROPERTY(meta = (BindWidget))
	UTextBlock* ScoreAmount;
	
	UPROPERTY(meta = (BindWidget))
	UTextBlock* DefeatsAmount;
	
	UPROPERTY(meta = (BindWidget))
	UTextBlock* WeaponAmmoAmount;
	
	UPROPERTY(meta = (BindWidget))
	UTextBlock* CarriedAmmoAmount;
	
	UPROPERTY(meta = (BindWidget))
	UTextBlock* MatchCountdownText;

	UPROPERTY(meta = (BindWidget))
	class UImage* HighPingImage;
	
	UPROPERTY(meta = (BindWidget))
	UTextBlock* GrenadesText;

	UPROPERTY(meta = (BindWidget))
	class UElimAnnouncement* ElimAnnouncement;

	UPROPERTY(meta = (BindWidget))
	class UChatting* Chatting;

	void UpdateHealthBar(const float BarPercent) const;
	void UpdateHealthText(const FString& InString) const;
	void UpdateShieldBar(const float BarPercent) const;
	void UpdateShieldText(const FString& InString) const;
	void UpdateScoreAmount(const FString& InString) const;
	void UpdateDefeatsAmount(const FString& InString) const;
	void UpdateWeaponAmmoAmount(const FString& InString) const;
	void UpdateCarriedAmmoAmount(const FString& InString) const;
	void UpdateMatchCountdownText(const FString& InString) const;
	void UpdateGrenadesAmount(const FString& InString) const;
	void GenerateElimAnnouncement(const FString& ElimAnnouncementText);
	void SetChatMode();
	void GenerateChat(const FText& TextToChat);
	
	void StartHighPingAnimation();
	void StopHighPingAnimation();

	/**
	 * 매치 카운트다운 빨간색으로 점멸
	 */
	UPROPERTY()
	FTimeline MatchCountdownColorTimeline;
	FOnTimelineLinearColor UpdateColorTrack;

	UPROPERTY(EditAnywhere)
	UCurveLinearColor* MatchCountdownColorCurve;
	
	void LerpMatchCountdownTextColor();

	UFUNCTION()
	void UpdateColorLerp(const FLinearColor Color);

	/**
	 * 핑 높을 시 이미지 점멸
	 */

	UPROPERTY(meta = (BindWidgetAnim), Transient)
	UWidgetAnimation* HighPingAnimation;

protected:
	virtual void NativeTick(const FGeometry& MyGeometry, float InDeltaTime) override;
};
