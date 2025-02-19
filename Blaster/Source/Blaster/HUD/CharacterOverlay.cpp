#include "CharacterOverlay.h"

#include "Components/ProgressBar.h"
#include "Components/TextBlock.h"

void UCharacterOverlay::UpdateHealthBar(const float BarPercent) const
{
	HealthBar->SetPercent(BarPercent);
}

void UCharacterOverlay::UpdateHealthText(const FString& HealthText) const
{
	HealthTextBlock->SetText(FText::FromString(HealthText));
}

void UCharacterOverlay::UpdateScoreAmount(const FString& String) const
{
	ScoreAmount->SetText(FText::FromString(String));
}

void UCharacterOverlay::UpdateDefeatsAmount(const FString& String) const
{
	DefeatsAmount->SetText(FText::FromString(String));
}

void UCharacterOverlay::UpdateWeaponAmmoAmount(const FString& String) const
{
	WeaponAmmoAmount->SetText(FText::FromString(String));
}

void UCharacterOverlay::UpdateCarriedAmmoAmount(const FString& String) const
{
	CarriedAmmoAmount->SetText(FText::FromString(String));
}

void UCharacterOverlay::UpdateMatchCountdownText(const FString& String) const
{
	MatchCountdownText->SetText(FText::FromString(String));
}
