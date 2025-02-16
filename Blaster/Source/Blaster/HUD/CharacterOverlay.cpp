#include "CharacterOverlay.h"

#include "Components/ProgressBar.h"
#include "Components/TextBlock.h"

void UCharacterOverlay::UpdateHealthBar(const float BarPercent)
{
	HealthBar->SetPercent(BarPercent);
}

void UCharacterOverlay::UpdateHealthText(FString HealthText)
{
	HealthTextBlock->SetText(FText::FromString(HealthText));
}

void UCharacterOverlay::UpdateScoreAmount(const FString& String)
{
	ScoreAmount->SetText(FText::FromString(String));
}

void UCharacterOverlay::UpdateDefeatsAmount(const FString& String)
{
	DefeatsAmount->SetText(FText::FromString(String));
}
