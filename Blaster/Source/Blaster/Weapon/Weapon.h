#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "WeaponTypes.h"
#include "Weapon.generated.h"

UENUM(BlueprintType)
enum class EWeaponState : uint8
{
	EWS_Initial UMETA(DisplayName = "Initial State"),
	EWS_Equipped UMETA(DisplayName = "Equipped"),
	EWS_Dropped UMETA(DisplayName = "Dropped"),
	
	EWS_MAX UMETA(DisplayName = "DefaultMAX"),
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
	void Dropped();
	void AddAmmo(int32 AmmoToAdd);

	/**
	 * Textures for the weapon crosshairs, 무기마다 다른 크로스헤어 지원할 수 있게 EditAnywhere
	 */

	UPROPERTY(EditAnywhere, Category = "Crosshairs")
	UTexture2D* CrosshairsCenter;
	
	UPROPERTY(EditAnywhere, Category = "Crosshairs")
	UTexture2D* CrosshairsLeft;
	
	UPROPERTY(EditAnywhere, Category = "Crosshairs")
	UTexture2D* CrosshairsRight;
	
	UPROPERTY(EditAnywhere, Category = "Crosshairs")
	UTexture2D* CrosshairsTop;
	
	UPROPERTY(EditAnywhere, Category = "Crosshairs")
	UTexture2D* CrosshairsBottom;

	/**
	 * Zoomed FOV while aiming
	 */

	UPROPERTY(EditAnywhere)
	float ZoomedFOV = 30.f;

	UPROPERTY(EditAnywhere)
	float ZoomInterpSpeed = 20.f;

	/**
	 * 크로스헤어 스프레드 관련 변수
	 */

	// 조준 사격 정확도, 높을수록 이동하거나 점프 도중에도 정확도가 상승
	UPROPERTY(EditAnywhere, Category = "Crosshairs")
	float ZoomAccurate = 0.6f;

	// 비조준 사격 시 하락하는 정확도, 높을수록 크게 빗나감
	UPROPERTY(EditAnywhere, Category = "Crosshairs")
	float HipFireAccurateSubtract = 0.75f;
	
	// 비조준 사격 시 하락하는 정확도의 최대치
	UPROPERTY(EditAnywhere, Category = "Crosshairs")
	float HipFireAccurateMaxSubtract = 3.f;

	/**
	 * Automatic fire
	 */
	UPROPERTY(EditAnywhere, Category = "Combat")
	float FireDelay = 0.15f;

	UPROPERTY(EditAnywhere, Category = "Combat")
	bool bAutomatic = true;

	UPROPERTY(EditAnywhere, Category = "Combat")
	class USoundCue* EquipSound;
	
	
protected:
	virtual void BeginPlay() override;

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

private:
	UPROPERTY(VisibleAnywhere, Category = "Weapon Properties")
	USkeletalMeshComponent* WeaponMesh;

	UPROPERTY(VisibleAnywhere, Category = "Weapon Properties")
	class USphereComponent* AreaSphere;

	UPROPERTY(ReplicatedUsing = OnRep_WeaponState, VisibleAnywhere, Category = "Weapon Properties")
	EWeaponState WeaponState;

	// WeaponState가 바뀌면 호출되는 콜백 함수
	UFUNCTION()
	void OnRep_WeaponState();

	UPROPERTY(VisibleAnywhere, Category = "Weapon Properties")
	class UWidgetComponent* PickupWidget;

	UPROPERTY(EditAnywhere, Category = "Weapon Properties")
	class UAnimationAsset* FireAnimation;

	UPROPERTY(EditAnywhere)
	TSubclassOf<class ACasing> CasingClass;

	UPROPERTY(EditAnywhere, ReplicatedUsing=OnRep_Ammo)
	int32 Ammo;

	UFUNCTION()
	void OnRep_Ammo();

	void SpendRound();

	// 최대 탄창
	UPROPERTY(EditAnywhere)
	int32 MagCapacity;

	UPROPERTY()
	class ABlasterCharacter* BlasterOwnerCharacter;
	UPROPERTY()
	class ABlasterPlayerController* BlasterOwnerController;

	UPROPERTY(EditAnywhere)
	EWeaponType WeaponType;
	
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
	FORCEINLINE EWeaponType GetWeaponType() const { return WeaponType; }
	FORCEINLINE int32 GetAmmo() const { return Ammo; }
	FORCEINLINE int32 GetMagCapacity() const { return MagCapacity; }
};
