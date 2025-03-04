#pragma once

#include "CoreMinimal.h"
#include "WeaponTypes.generated.h"

#define TRACE_LENGTH 80000.f;
#define CUSTOM_DEPTH_PURPLE 250
#define CUSTOM_DEPTH_BLUE 251
#define CUSTOM_DEPTH_TAN 252

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