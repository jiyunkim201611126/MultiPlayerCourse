#pragma once

#include "CoreMinimal.h"
#include "WeaponTypes.generated.h"

#define TRACE_LENGTH 80000.f;
#define CUSTOM_DEPTH_WHITE 0
#define CUSTOM_DEPTH_BLUE 1
#define CUSTOM_DEPTH_PURPLE 2

UENUM(BlueprintType)
enum class EWeaponType : uint8
{
	EWT_AssaultRifle UMETA(DisplayName = "Assault Rifle"),
	EWT_RocketLauncher UMETA(DisplayName = "Rocket Launcher"),
	EWT_Pistol UMETA(DisplayName = "Pistol"),
	EWT_SubmachineGun UMETA(DisplayName = "SubmachineGun"),
	EWT_Shotgun UMETA(DisplayName = "Shotgun"),
	EWT_SniperRifle UMETA(DisplayName = "Sniper Rifle"),
	EWT_GrenadeLauncher UMETA(DisplayName = "Grenade Launcher"),
	
	EWT_MAX UMETA(DisplayName = "DefualtMax"),
};


UENUM(BlueprintType)
enum class EWeaponGrade : uint8
{
	EWG_Common = CUSTOM_DEPTH_WHITE			UMETA(DisplayName = "Common"),
	EWG_Rare = CUSTOM_DEPTH_BLUE			UMETA(DisplayName = "Rare"),
	EWG_Legendary = CUSTOM_DEPTH_PURPLE		UMETA(DisplayName = "Legendary"),

	EWG_MAX UMETA(DisplayName = "DefualtMax"),
};

USTRUCT()
struct BLASTER_API FWeaponTypes
{
	GENERATED_BODY()

	FWeaponTypes()
	{
		WeaponName.Add(EWeaponType::EWT_AssaultRifle, TEXT("Rifle"));
		WeaponName.Add(EWeaponType::EWT_RocketLauncher, TEXT("RocketLauncher"));
		WeaponName.Add(EWeaponType::EWT_Pistol, TEXT("Pistol"));
		WeaponName.Add(EWeaponType::EWT_SubmachineGun, TEXT("Pistol"));
		WeaponName.Add(EWeaponType::EWT_Shotgun, TEXT("Shotgun"));
		WeaponName.Add(EWeaponType::EWT_SniperRifle, TEXT("SniperRifle"));
		WeaponName.Add(EWeaponType::EWT_GrenadeLauncher, TEXT("GrenadeLauncher"));
	}

	static const FWeaponTypes& GetWeaponTypeInstance()
	{
		static FWeaponTypes Instance;
		return Instance;
	}

	UPROPERTY()
	TMap<EWeaponType, FName> WeaponName;
};