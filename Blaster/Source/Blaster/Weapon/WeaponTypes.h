#pragma once

#include "CoreMinimal.h"
#include "WeaponTypes.generated.h"

#define TRACE_LENGTH 80000.f;

UENUM(BlueprintType)
enum class EWeaponType : uint8
{
	EWT_AssaultRifle UMETA(DisplayName = "Assault Rifle"),
	EWT_RocketLauncher UMETA(DisplayName = "Rocket Launcher"),
	EWT_Pistol UMETA(DisplayName = "Pistol"),
	EWT_SubmachineGun UMETA(DisplayName = "SubmachineGun"),
	EWT_Shotgun UMETA(DisplayName = "Shotgun"),
	EWT_SniperRifle UMETA(DisplayName = "Sniper Rifle"),
	
	EWT_MAX UMETA(DisplayName = "DefualtMax"),
};

USTRUCT()
struct BLASTER_API FWeaponTypes
{
	GENERATED_BODY()

	FWeaponTypes()
	{
		WeaponName.Add(EWeaponType::EWT_AssaultRifle, TEXT("Rifle"));
		WeaponName.Add(EWeaponType::EWT_RocketLauncher, TEXT("Rifle"));
		WeaponName.Add(EWeaponType::EWT_Pistol, TEXT("Rifle"));
		WeaponName.Add(EWeaponType::EWT_SubmachineGun, TEXT("Rifle"));
		WeaponName.Add(EWeaponType::EWT_Shotgun, TEXT("Rifle"));
		WeaponName.Add(EWeaponType::EWT_SniperRifle, TEXT("Rifle"));
	}

	static const FWeaponTypes& GetWeaponTypeInstance()
	{
		static FWeaponTypes Instance;
		return Instance;
	}

	UPROPERTY()
	TMap<EWeaponType, FName> WeaponName;
};