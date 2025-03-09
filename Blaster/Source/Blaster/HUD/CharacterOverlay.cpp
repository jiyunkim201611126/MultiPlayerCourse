#include "CharacterOverlay.h"

#include "Components/ProgressBar.h"
#include "Components/TextBlock.h"

void UCharacterOverlay::UpdateHealthBar(const float BarPercent) const
{
	if (HealthBar)
	{
		HealthBar->SetPercent(BarPercent);
	}
}

void UCharacterOverlay::UpdateHealthText(const FString& InString) const
{
	if (HealthTextBlock)
	{
		HealthTextBlock->SetText(FText::FromString(InString));
	}
}

void UCharacterOverlay::UpdateShieldBar(const float BarPercent) const
{
	if (ShieldBar)
	{
		ShieldBar->SetPercent(BarPercent);
	}
}

void UCharacterOverlay::UpdateShieldText(const FString& InString) const
{
	if (ShieldTextBlock)
	{
		ShieldTextBlock->SetText(FText::FromString(InString));
	}
}

void UCharacterOverlay::UpdateScoreAmount(const FString& InString) const
{
	if (ScoreAmount)
	{
		ScoreAmount->SetText(FText::FromString(InString));
	}
}

void UCharacterOverlay::UpdateDefeatsAmount(const FString& InString) const
{
	if (DefeatsAmount)
	{
		DefeatsAmount->SetText(FText::FromString(InString));
	}
}

void UCharacterOverlay::UpdateWeaponAmmoAmount(const FString& InString) const
{
	if (WeaponAmmoAmount)
	{
		WeaponAmmoAmount->SetText(FText::FromString(InString));
	}
}

void UCharacterOverlay::UpdateCarriedAmmoAmount(const FString& InString) const
{
	if (CarriedAmmoAmount)
	{
		CarriedAmmoAmount->SetText(FText::FromString(InString));
	}
}

void UCharacterOverlay::UpdateMatchCountdownText(const FString& InString) const
{
	if (MatchCountdownText)
	{
		MatchCountdownText->SetText(FText::FromString(InString));
	}
}

void UCharacterOverlay::UpdateGrenadesAmount(const FString& InString) const
{
	if (GrenadesText)
	{
		GrenadesText->SetText(FText::FromString(InString));
	}
}

void UCharacterOverlay::LerpMatchCountdownTextColor()
{
	// 첫 진입 시(바인딩 상태로 체크) 업데이트 함수 바인딩
	if (!UpdateColorTrack.IsBound() && MatchCountdownColorCurve)
	{
		UpdateColorTrack.BindDynamic(this, &ThisClass::UpdateColorLerp);

		MatchCountdownColorTimeline.AddInterpLinearColor(MatchCountdownColorCurve, UpdateColorTrack);
		MatchCountdownColorTimeline.SetTimelineLengthMode(TL_TimelineLength);
	}

	// 빨간색으로 변경 후 타임라인 시작
	if (MatchCountdownText)
	{
		MatchCountdownText->SetColorAndOpacity(FSlateColor(FLinearColor::Red));
		MatchCountdownColorTimeline.PlayFromStart();
	}
}

void UCharacterOverlay::UpdateColorLerp(const FLinearColor Color)
{
	if (MatchCountdownText)
	{
		MatchCountdownText->SetColorAndOpacity(FSlateColor(Color));
	}
}

void UCharacterOverlay::NativeTick(const FGeometry& MyGeometry, float InDeltaTime)
{
	Super::NativeTick(MyGeometry, InDeltaTime);

	// FTimeline은 스스로 갱신하지 못 하므로 Tick에서 호출
	if (UpdateColorTrack.IsBound())
	{
		MatchCountdownColorTimeline.TickTimeline(InDeltaTime);
	}
}
