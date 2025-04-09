#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "WeaponTypes.h"
#include "Weapon.generated.h"

DECLARE_DELEGATE(FOnFireTimerFinished);

UENUM(BlueprintType)
enum class EWeaponState : uint8
{
	EWS_Initial UMETA(DisplayName = "Initial State"),
	EWS_Equipped UMETA(DisplayName = "Equipped"),
	EWS_EquippedSecondary UMETA(DisplayName = "Equipped Secondary"),
	EWS_Dropped UMETA(DisplayName = "Dropped"),
	
	EWS_MAX UMETA(DisplayName = "DefaultMAX"),
};

UENUM(BlueprintType)
enum class EFireType : uint8
{
	EFT_HitScan UMETA(DisplayName = "Hit Scan Weapon"),
	EFT_Projectile UMETA(DisplayName = "Projectile Weapon"),
	EFT_Shotgun UMETA(DisplayName = "Shotgun Weapon"),
	
	EFT_MAX UMETA(DisplayName = "DefaultMAX"),
};

UCLASS()
class BLASTER_API AWeapon : public AActor
{
	GENERATED_BODY()
	
public:	
	AWeapon();
	virtual void Tick(float DeltaTime) override;
	virtual void GetLifetimeReplicatedProps(TArray<class FLifetimeProperty>& OutLifetimeProps) const override;
	virtual void OnRep_Owner() override;
	void SetHUDAmmo();

	void ShowPickupWidget(const bool bShowWidget);
	void PlayFireMontage() const;
	virtual void Fire(const FVector& HitTarget);
	bool bCanFire = true;
	void Dropped();
	void AddAmmo(int32 AmmoToAdd);
	
	// 탄퍼짐을 적용한 수치로 라인 트레이스의 End 벡터를 계산하는 함수
	FVector TraceEndWithScatter(const FVector& HitTarget);

	/**
	 * Textures for the weapon crosshairs, 무기마다 다른 크로스헤어 지원할 수 있게 EditAnywhere
	 */

	UPROPERTY(EditAnywhere, Category = "Weapon Properties | Crosshairs")
	UTexture2D* CrosshairsCenter;
	
	UPROPERTY(EditAnywhere, Category = "Weapon Properties | Crosshairs")
	UTexture2D* CrosshairsLeft;
	
	UPROPERTY(EditAnywhere, Category = "Weapon Properties | Crosshairs")
	UTexture2D* CrosshairsRight;
	
	UPROPERTY(EditAnywhere, Category = "Weapon Properties | Crosshairs")
	UTexture2D* CrosshairsTop;
	
	UPROPERTY(EditAnywhere, Category = "Weapon Properties | Crosshairs")
	UTexture2D* CrosshairsBottom;

	/**
	 * Zoomed FOV while aiming
	 */

	UPROPERTY(EditAnywhere, Category = "Combat | Zoom")
	float ZoomedFOV = 30.f;

	UPROPERTY(EditAnywhere, Category = "Combat | Zoom")
	float ZoomInterpSpeed = 20.f;

	/**
	 * 크로스헤어 스프레드, 탄퍼짐 관련 변수
	 */
	
	// 탄퍼짐 사용 여부를 결정하는 bool.
	UPROPERTY(EditAnywhere, Category = "Combat | Weapon Scatter")
	bool bUseScatter = false;
	
	// 조준 사격 정확도, 높을수록 이동하거나 점프 도중에도 정확도가 상승
	UPROPERTY(EditAnywhere, Category = "Combat | Weapon Scatter", meta = (editcondition = "bUseScatter"))
	float ZoomAccurate = 0.6f;

	// 비조준 사격 시 하락하는 정확도, 높을수록 크게 빗나감
	UPROPERTY(EditAnywhere, Category = "Combat | Weapon Scatter", meta = (editcondition = "bUseScatter"))
	float HipFireAccurateSubtract = 0.75f;
	
	// 비조준 사격 시 하락하는 정확도의 최대치
	UPROPERTY(EditAnywhere, Category = "Combat | Weapon Scatter", meta = (editcondition = "bUseScatter"))
	float HipFireAccurateMaxSubtract = 3.f;

	// 기본 탄퍼짐
	UPROPERTY(EditAnywhere, Category = "Combat | Weapon Scatter")
	float DefaultSpreadFactor = 0.f;
	
	// 크로스헤어 스프레드 상태에 따라 더해지는 탄퍼짐 정도
	UPROPERTY(VisibleAnywhere, Category = "Combat | Weapon Scatter")
	float AddSpreadFactor = 0.f;
	
	// 탄퍼짐의 기준이 되는 구체의 거리. 클수록 탄퍼짐이 좁아짐
	UPROPERTY(EditAnywhere, Category = "Combat | Weapon Scatter")
	float DistanceToSphere = 350.f;

	// 조준 시 사격에 의한 탄퍼짐이 줄어드는 속도
	UPROPERTY(EditAnywhere, Category = "Combat | Weapon Scatter")
	float AimingSpreadRecoverySpeed = 20.f;

	// 비조준 시 사격에 의한 탄퍼짐이 줄어드는 속도
	UPROPERTY(EditAnywhere, Category = "Combat | Weapon Scatter")
	float HipSpreadRecoverySpeed = 10.f;

	/**
	 * Automatic fire
	 */
	UPROPERTY(EditAnywhere, Category = "Combat | Combat")
	float FireDelay = 0.15f;

	UPROPERTY(EditAnywhere, Category = "Combat | Combat")
	bool bAutomatic = true;
	
	// 타이머 추적용
	FTimerHandle FireTimer;

	void StartFireTimer();
	void FireTimerFinished();

	FOnFireTimerFinished OnFireTimerFinished;

	UPROPERTY(EditAnywhere)
	class USoundCue* EquipSound;

	/**
	 * Enable or disable custom depth
	 */

	void EnableCustomDepth(bool bEnable);

	UPROPERTY(EditAnywhere)
	EWeaponGrade WeaponGrade = EWeaponGrade::EWG_Common;

	bool bDestroyWeapon = false;

	UPROPERTY(EditAnywhere)
	EFireType FireType;

	// Ammo 업데이트를 위해 서버에 요청한 패킷 카운트
	int32 Sequence = 0;

	UPROPERTY(EditAnywhere)
	float Damage = 0.f;

	UPROPERTY(Replicated, EditAnywhere)
	bool bUseServerSideRewind = false;

	UFUNCTION()
	void OnPingTooHigh(bool bPingTooHigh);
	
protected:
	virtual void OnConstruction(const FTransform& Transform) override;
	virtual void BeginPlay() override;
	virtual void OnWeaponStateSet();
	virtual void OnEquipped();
	virtual void OnEquippedSecondary();
	virtual void OnDropped();

	UFUNCTION()
	virtual void OnSphereOverlap(
		UPrimitiveComponent* OverlappedComponent,
		AActor* OtherActor,
		UPrimitiveComponent* OtherComp,
		int32 OtherBodyIndex,
		bool bFromSweep,
		const FHitResult& SweepResult
		);

	UFUNCTION()
	void OnSphereEndOverlap(
		UPrimitiveComponent* OverlappedComponent,
		AActor* OtherActor,
		UPrimitiveComponent* OtherComp,
		int32 OtherBodyIndex
		);

	UPROPERTY()
	class ABlasterCharacter* BlasterOwnerCharacter;
	UPROPERTY()
	class ABlasterPlayerController* BlasterOwnerController;
	
private:
	UPROPERTY(VisibleAnywhere, Category = "Weapon Properties | Weapon Properties")
	USkeletalMeshComponent* WeaponMesh;

	UPROPERTY(VisibleAnywhere, Category = "Weapon Properties | Weapon Properties")
	class USphereComponent* AreaSphere;

	UPROPERTY(ReplicatedUsing = OnRep_WeaponState, VisibleAnywhere, Category = "Weapon Properties | Weapon Properties")
	EWeaponState WeaponState;

	// WeaponState가 바뀌면 호출되는 콜백 함수
	UFUNCTION()
	void OnRep_WeaponState();

	UPROPERTY(VisibleAnywhere, Category = "Weapon Properties | Weapon Properties")
	class UWidgetComponent* PickupWidget;

	UPROPERTY(EditAnywhere)
	UAnimationAsset* FireAnimation;

	UPROPERTY(EditAnywhere)
	TSubclassOf<class ACasing> CasingClass;

	UPROPERTY(EditAnywhere)
	int32 Ammo;

	UFUNCTION(Client, Reliable)
	void ClientUpdateAmmoWhenShot(int32 ServerAmmo);

	UFUNCTION(Client, Reliable)
	void ClientAddAmmo(int32 ReloadedAmmo, int32 AmmoToAdd);

	void SpendRound();

	UFUNCTION(Client, Reliable)
	void ClientUpdateAmmoWhenEquip(int32 ServerAmmo);

	// 최대 탄창
	UPROPERTY(EditAnywhere)
	int32 MagCapacity;

	UPROPERTY(EditAnywhere)
	EWeaponType WeaponType;

	UPROPERTY(EditAnywhere)
	bool bNeedPhysicsSimulate = false;
	
public:
	void SetWeaponState(EWeaponState State);
	FORCEINLINE USphereComponent* GetAreaSphere() const { return AreaSphere; }
	FORCEINLINE USkeletalMeshComponent* GetWeaponMesh() const { return WeaponMesh; }
	FORCEINLINE float GetZoomedFOV() const { return ZoomedFOV; }
	FORCEINLINE float GetZoomInterpSpeed() const { return ZoomInterpSpeed; }
	FORCEINLINE float GetZoomAccurate() const { return ZoomAccurate; }
	FORCEINLINE float GetHipFireAccurateSubtract() const { return HipFireAccurateSubtract; }
	FORCEINLINE float GetHipFireAccurateMaxSubtract() const { return HipFireAccurateMaxSubtract; }
	bool IsEmpty();
	bool IsFull();
	FORCEINLINE EWeaponType GetWeaponType() const { return WeaponType; }
	FORCEINLINE int32 GetAmmo() const { return Ammo; }
	FORCEINLINE int32 GetMagCapacity() const { return MagCapacity; }
	FORCEINLINE float GetDamage() const { return Damage; }
};
