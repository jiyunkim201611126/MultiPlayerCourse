#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Blaster/HUD/BlasterHUD.h"
#include "Blaster/Weapon/WeaponTypes.h"
#include "Blaster/BlasterTypes/CombatState.h"
#include "CombatComponent.generated.h"

UCLASS( ClassGroup=(Custom), meta=(BlueprintSpawnableComponent) )
class BLASTER_API UCombatComponent : public UActorComponent
{
	GENERATED_BODY()

public:	
	UCombatComponent();
	friend class ABlasterCharacter;
	virtual void GetLifetimeReplicatedProps(TArray<class FLifetimeProperty>& OutLifetimeProps) const override;
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

	void EquipWeapon(class AWeapon* WeaponToEquip);
	void Reload();

	// 애니메이션 재생 중 ReloadFinished 노티파이를 만나면 호출되는 함수
	UFUNCTION(BlueprintCallable)
	void FinishReloading();

	void FireButtonPressed(bool bPressed);

	// 샷건 재장전 애니메이션 재생 중 Shell 노티파이를 만나면 호출되는 함수
	UFUNCTION(BlueprintCallable)
	void ShotgunShellReload();

	void JumpToShotgunMoreReload(bool bNeedMoreReload);

	UFUNCTION(BlueprintCallable)
	void ThrowGrenadeFinished();

protected:
	virtual void BeginPlay() override;
	void SetAiming(bool bIsAiming);

	UFUNCTION(Server, Reliable)
	void ServerSetAiming(bool bIsAiming);

	UFUNCTION()
	void OnRep_EquippedWeapon();
	
	void Fire();

	void LocalFire(const FVector_NetQuantize& TraceHitTarget);

	UFUNCTION(Server, Reliable)
	void ServerFire(const FVector_NetQuantize& TraceHitTarget);

	UFUNCTION(NetMulticast, Reliable)
	void MulticastFire(const FVector_NetQuantize& TraceHitTarget);

	FVector_NetQuantize TraceUnderCrosshairs(FHitResult& TraceHitResult);

	void SetHUDCrosshairs(float DeltaTime);

	UFUNCTION(Server, Reliable)
	void ServerReload();

	void HandleReload();
	// 장전해야 하는 Ammo의 수를 구하는 함수
	int32 AmountToReload();

	void ThrowGrenade();

	UFUNCTION(Server, Reliable)
	void ServerThrowGrenade();

private:
	UPROPERTY()
	ABlasterCharacter* Character;

	// 크로스헤어를 그리기 위한 Controller와 HUD 선언
	UPROPERTY()
	class ABlasterPlayerController* Controller;
	UPROPERTY()
	class ABlasterHUD* HUD;

	// 상태가 변경되면 클라이언트에서만 OnRep_EquippedWeapon이 자동으로 실행됨
	UPROPERTY(ReplicatedUsing = OnRep_EquippedWeapon)
	AWeapon* EquippedWeapon;

	UPROPERTY(Replicated)
	bool bAiming;

	UPROPERTY(EditAnywhere)
	float BaseWalkSpeed;
	
	UPROPERTY(EditAnywhere)
	float AimWalkSpeed;

	UPROPERTY()
	bool bFireButtonPressed;	

	/**
	 * HUD and crosshairs
	 */
	float CrosshairVelocityFactor = 0.f;
	float CrosshairInAirFactor = 0.f;
	float CrosshairAimFactor = 0.f;
	float CrosshairShootingFactor = 0.f;

	FVector HitTarget;
	
	FHUDPackage HUDPackage;

	/**
	 * Aiming and FOV
	 */
	// 카메라 Field of view; BeginPlay에서 카메라의 base FOV를 설정
	float DefaultFOV;
	float CurrentFOV;

	void InterpFOV(float DeltaTime);

	/**
	 * Automatic fire
	 */
	// 타이머 추적용
	FTimerHandle FireTimer;
	bool bCanFire = true;

	void StartFireTimer();
	void FireTimerFinished();

	bool CanFire();

	// 현재 총기 종류에 해당하는 남은 탄 수
	UPROPERTY(ReplicatedUsing=OnRep_CarriedAmmo)
	int32 CarriedAmmo;

	UFUNCTION()
	void OnRep_CarriedAmmo();

	// 현재 캐릭터가 가지고 있는 총기 종류별 탄 수
	TMap<EWeaponType, int32> CarriedAmmoMap;

	UPROPERTY(EditAnywhere)
	int32 StartingARAmmo = 0;

	UPROPERTY(EditAnywhere)
	int32 StartingRocketAmmo = 0;

	UPROPERTY(EditAnywhere)
	int32 StartingPistolAmmo = 0;

	UPROPERTY(EditAnywhere)
	int32 StartingSMGAmmo = 0;

	UPROPERTY(EditAnywhere)
	int32 StartingShotgunAmmo = 0;

	UPROPERTY(EditAnywhere)
	int32 StartingSniperAmmo = 0;

	UPROPERTY(EditAnywhere)
	int32 StartingGrenadeLauncherAmmo = 0;

	// CarriedAmmoMap 초기화
	void InitializeCarriedAmmo();

	UPROPERTY(ReplicatedUsing=OnRep_CombatState)
	ECombatState CombatState = ECombatState::ECS_Unoccupied;

	UFUNCTION()
	void OnRep_CombatState();
	
	// 장전 완료에 따른 탄알 수 조정
	void UpdateAmmoValues();
	void UpdateShotgunAmmoValues();
};
