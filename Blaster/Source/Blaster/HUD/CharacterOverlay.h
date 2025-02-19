#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
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
	UTextBlock* ScoreAmount;
	
	UPROPERTY(meta = (BindWidget))
	UTextBlock* DefeatsAmount;
	
	UPROPERTY(meta = (BindWidget))
	UTextBlock* WeaponAmmoAmount;
	
	UPROPERTY(meta = (BindWidget))
	UTextBlock* CarriedAmmoAmount;
	
	UPROPERTY(meta = (BindWidget))
	UTextBlock* MatchCountdownText;

	void UpdateHealthBar(const float BarPercent) const;
	void UpdateHealthText(const FString& HealthText) const;
	void UpdateScoreAmount(const FString& String) const;
	void UpdateDefeatsAmount(const FString& String) const;
	void UpdateWeaponAmmoAmount(const FString& String) const;
	void UpdateCarriedAmmoAmount(const FString& String) const;
	void UpdateMatchCountdownText(const FString& String) const;
};
