#include "CharacterOverlay.h"

#include "Components/ProgressBar.h"
#include "Components/TextBlock.h"

void UCharacterOverlay::UpdateHealthBar(const float BarPercent) const
{
	HealthBar->SetPercent(BarPercent);
}

void UCharacterOverlay::UpdateHealthText(const FString& InString) const
{
	HealthTextBlock->SetText(FText::FromString(InString));
}

void UCharacterOverlay::UpdateScoreAmount(const FString& InString) const
{
	ScoreAmount->SetText(FText::FromString(InString));
}

void UCharacterOverlay::UpdateDefeatsAmount(const FString& InString) const
{
	DefeatsAmount->SetText(FText::FromString(InString));
}

void UCharacterOverlay::UpdateWeaponAmmoAmount(const FString& InString) const
{
	WeaponAmmoAmount->SetText(FText::FromString(InString));
}

void UCharacterOverlay::UpdateCarriedAmmoAmount(const FString& InString) const
{
	CarriedAmmoAmount->SetText(FText::FromString(InString));
}

void UCharacterOverlay::UpdateMatchCountdownText(const FString& InString) const
{
	MatchCountdownText->SetText(FText::FromString(InString));
}
