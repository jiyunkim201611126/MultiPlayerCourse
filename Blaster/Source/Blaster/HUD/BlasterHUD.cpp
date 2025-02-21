#include "BlasterHUD.h"

#include "CharacterOverlay.h"

void ABlasterHUD::BeginPlay()
{
	Super::BeginPlay();
}

void ABlasterHUD::AddCharacterOverlay()
{
	APlayerController* PlayerController = GetOwningPlayerController();
	if (PlayerController && CharacterOverlayClass)
	{
		CharacterOverlay = CreateWidget<UCharacterOverlay>(PlayerController, CharacterOverlayClass);
		CharacterOverlay->AddToViewport();
	}
}

void ABlasterHUD::DrawHUD()
{
	Super::DrawHUD();
	
	const FVector2D ViewportCenter = GetCrosshairLocation();

	const float SpreadScaled = CrosshairSpreadMax * HUDPackage.CrosshairSpread;
	if (HUDPackage.CrosshairsCenter)
	{
		const FVector2D Spread(0.f, 0.f);
		DrawCrosshairs(HUDPackage.CrosshairsCenter, ViewportCenter, Spread, HUDPackage.CrosshairsColor);
	}
	if (HUDPackage.CrosshairsLeft)
	{
		const FVector2D Spread(-SpreadScaled, 0.f);
		DrawCrosshairs(HUDPackage.CrosshairsLeft, ViewportCenter, Spread, HUDPackage.CrosshairsColor);
	}
	if (HUDPackage.CrosshairsRight)
	{
		const FVector2D Spread(SpreadScaled, 0.f);
		DrawCrosshairs(HUDPackage.CrosshairsRight, ViewportCenter, Spread, HUDPackage.CrosshairsColor);
	}
	if (HUDPackage.CrosshairsTop)
	{
		const FVector2D Spread(0.f, -SpreadScaled);
		DrawCrosshairs(HUDPackage.CrosshairsTop, ViewportCenter, Spread, HUDPackage.CrosshairsColor);
	}
	if (HUDPackage.CrosshairsBottom)
	{
		const FVector2D Spread(0.f, SpreadScaled);
		DrawCrosshairs(HUDPackage.CrosshairsBottom, ViewportCenter, Spread, HUDPackage.CrosshairsColor);
	}
}

FVector2D ABlasterHUD::GetCrosshairLocation()
{
	if (GEngine)
	{
		FVector2D ViewportSize;
		GEngine->GameViewport->GetViewportSize(ViewportSize);
		const FVector2D ViewportCenter(ViewportSize.X / 2.f, ViewportSize.Y / 2.f);
		return ViewportCenter;
	}

	return FVector2D::ZeroVector;
}

void ABlasterHUD::DrawCrosshairs(UTexture2D* Texture, FVector2D ViewportCenter, FVector2D Spread, FLinearColor CrosshairsColor)
{
	const float TextureWidth = Texture->GetSizeX();
	const float TextureHeight = Texture->GetSizeY();
	const FVector2D TextureDrawPoint(
		ViewportCenter.X - (TextureWidth / 2.f) + Spread.X,
		ViewportCenter.Y - (TextureHeight / 2.f) + Spread.Y
		);

	DrawTexture(
		Texture,
		TextureDrawPoint.X,
		TextureDrawPoint.Y,
		TextureWidth,
		TextureHeight,
		0.f,
		0.f,
		1.f,
		1.f,
		CrosshairsColor
		);
}
