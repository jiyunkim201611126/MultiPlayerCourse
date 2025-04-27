#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "Blaster/BlasterTypes/TurningInPlace.h"
#include "Blaster/Interfaces/InteractWithCrosshairsInterface.h"
#include "Components/TimelineComponent.h"
#include "Blaster/BlasterTypes/CombatState.h"
#include "BlasterCharacter.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnLeftGame);

UCLASS()
class BLASTER_API ABlasterCharacter : public ACharacter, public IInteractWithCrosshairsInterface
{
	GENERATED_BODY()

public:
	ABlasterCharacter();
	virtual void PostInitializeComponents() override;
	virtual void SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent) override;
	virtual void GetLifetimeReplicatedProps(TArray<class FLifetimeProperty>& OutLifetimeProps) const override;
	virtual void PossessedBy(AController* NewController) override;
	virtual void OnRep_PlayerState() override;
	
	void Elim(bool bPlayerLeftGame);
	virtual void Destroyed() override;
	
	virtual void Tick(float DeltaTime) override;

	// AimOffset functions
	virtual void OnRep_ReplicatedMovement() override;
	void CalculateAO_Pitch();

	void EquipButtonPressed();
	void SwapButtonPressed();
	void CrouchButtonPressed();
	void AimButtonPressed();
	void AimButtonReleased();
	virtual void Jump() override;
	void FireButtonPressed();
	void FireButtonReleased();
	void ReloadButtonPressed();
	void GrenadeButtonPressed();
	
	void PlayFireMontage(bool bAiming);
	void PlayReloadMontage();
	void PlayElimMontage();
	void PlayThrowGrenadeMontage();
	void PlaySwapMontage();

	UFUNCTION(NetMulticast, Reliable)
	void MulticastPlayHitReactMontage(FVector_NetQuantize CausedLocation);

	UFUNCTION(BlueprintImplementableEvent)
	void ShowSniperScopeWidget(bool bShowScope);

	// Overlap될 때 변수 자체는 서버와 클라이언트 모두가 변경되지만, 콜백 함수는 겹친 클라이언트의 인스턴스에서만 호출.
	UPROPERTY(ReplicatedUsing = OnRep_OverlappingWeapons)
	TArray<class AWeapon*> OverlappingWeapons;
	
	void UpdateHUDHealth();
	void UpdateHUDShield();
	void UpdateHUDAmmo();

	void SpawnDefaultWeapon();

	// 클라이언트측에서 Swap 애니메이션을 제어하기 위한 변수
	bool bFinishedSwapping = true;

	// 클라이언트가 세션을 나갈 때 호출되는 함수
	UFUNCTION(Server, Reliable)
	void ServerLeaveGame();

	FOnLeftGame OnLeftGame;

	UFUNCTION(NetMulticast, Reliable)
	void MulticastGainedTheLead();

	UFUNCTION(NetMulticast, Reliable)
	void MulticastLostTheLead();

	void SetTeamColor(ETeam Team);
	
protected:
	virtual void BeginPlay() override;
	
	void AimOffset(float DeltaTime);
	void SimProxiesTurn();
	void RotateInPlace(float DeltaTime);
	
	UFUNCTION()
	void ReceiveDamage(AActor* DamagedActor, float Damage, const UDamageType* DamageType, class AController* InstigatorController, AActor* DamageCauser);
	
	// 시작 시 우상단 Score를 Refresh하기 위한 함수
	void PollInit();

	void OnPlayerStateInitialize(class ABlasterPlayerState* BlasterPlayerState);
	void SetSpawnPoint(ETeam Team);
	
private:
	UPROPERTY()
	class ABlasterPlayerController* BlasterPlayerController;
	
	UPROPERTY(VisibleAnywhere, Category = "Camera")
	class USpringArmComponent* CameraBoom;

	UPROPERTY(VisibleAnywhere, Category = "Camera")
	class UCameraComponent* FollowCamera;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, meta = (AllowPrivateAccess = "true"))
	class UWidgetComponent* OverheadWidget;
	
	void UpdatePlayerName() const;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, meta = (AllowPrivateAccess = "true"))
	class UCombatComponent* Combat;

	UPROPERTY(VisibleAnywhere)
	class UBuffComponent* Buff;

	UPROPERTY(VisibleAnywhere)
	class ULagCompensationComponent* LagCompensation;

	// Overlap될 때 클라이언트가 호출하는 콜백 함수
	UFUNCTION()
	void OnRep_OverlappingWeapons(TArray<AWeapon*> LastWeapons);

	// 클라이언트가 서버에 요청하는 함수, 패킷 손실돼도 다시 전송해 호출을 반드시 보장함.
	UFUNCTION(Server, Reliable)
	void ServerEquipButtonPressed();
	
	UFUNCTION(Server, Reliable)
	void ServerSwapButtonPressed();

	// AimOffset values
	float AO_Yaw;
	float InterpAO_Yaw;
	float AO_Pitch;
	FRotator StartingAimRotation;

	ETurningInPlace TurningInPlace;
	void TurnInPlace(float DeltaTime);

	/**
	 * Animation Montages
	 */
	
	UPROPERTY(EditAnywhere, Category = "Combat")
	class UAnimMontage* FireWeaponMontage;
	
	UPROPERTY(EditAnywhere, Category = "Combat")
	UAnimMontage* ReloadMontage;

	UPROPERTY(EditAnywhere, Category = "Combat")
	UAnimMontage* HitReactMontage;

	UPROPERTY(EditAnywhere, Category = "Combat")
	UAnimMontage* ElimMontage;
	
	UPROPERTY(EditAnywhere, Category = "Combat")
	UAnimMontage* ThrowGrenadeMontage;
	
	UPROPERTY(EditAnywhere, Category = "Combat")
	UAnimMontage* SwapMontage;

	void HideCameraIfCharacterClose();

	UPROPERTY(EditAnywhere)
	float CameraThreshold = 200.f;

	// Simulated proxies(내가 조작하지 않는 캐릭터)를 위한 AimOffset Value
	bool bRotateRootBone;
	float TurnThreshold = 0.5f;
	FRotator ProxyRotationLastFrame;
	FRotator ProxyRotation;
	float ProxyYaw;
	float TimeSinceLastMovementReplication;
	float CalculateSpeed();

	/**
	 * Player health 
	 */

	UPROPERTY(EditAnywhere, Category = "Player Stats")
	float MaxHealth = 100.f;

	UPROPERTY(ReplicatedUsing = OnRep_Health, VisibleAnywhere, Category = "Player Stats")
	float Health = 100.f;

	UFUNCTION()
	void OnRep_Health(float LastHealth);

	/**
	 * Player Shield 
	 */

	UPROPERTY(EditAnywhere, Category = "Player Stats")
	float MaxShield = 100.f;
	
	UPROPERTY(ReplicatedUsing = OnRep_Shield, EditAnywhere, Category = "Player Stats")
	float Shield = 0.f;

	UFUNCTION()
	void OnRep_Shield(float LastShield);

	/**
	 * Elim
	*/
	
	UFUNCTION(NetMulticast, Reliable)
	void MulticastElim(bool bPlayerLeftGame);
	
	bool bElimmed = false;

	FTimerHandle ElimTimer;

	UPROPERTY(EditDefaultsOnly)
	float ElimDelay = 3.f;

	void ElimTimerFinished();

	bool bLeftGame = false;

	// 무기 드랍
	void DropOrDestroyWeapons();
	void DropOrDestroyWeapon(class AWeapon* Weapon);

	/**
	 * Dissolve effect
	 */

	// 타임라인 컴포넌트와 델리게이트
	UPROPERTY(VisibleAnywhere)
	UTimelineComponent* DissolveTimeline;
	FOnTimelineFloat DissolveTrack;

	UPROPERTY(EditAnywhere)
	UCurveFloat* DissolveCurve;

	void StartDissolve();
	
	UFUNCTION()
	void UpdateDissolveMaterial(float DissolveValue);

	// 아래 머티리얼 인스턴스를 통해 만들어지는 런타임 중 편집 가능한 다이나믹 머티리얼 인스턴스
	UPROPERTY(VisibleAnywhere, Category = "Elim")
	UMaterialInstanceDynamic* DynamicDissolveMaterialInstance;

	// 머티리얼 인스턴스
	UPROPERTY(VisibleAnywhere, Category = "Elim")
	UMaterialInstance* DissolveMaterialInstance;

	/**
	 * 팀 색깔
	 */

	UPROPERTY(EditAnywhere, Category = "Elim")
	UMaterialInstance* RedDissolveMaterialInstance;
	
	UPROPERTY(EditAnywhere, Category = "Elim")
	UMaterialInstance* RedMaterialInstance;
	
	UPROPERTY(EditAnywhere, Category = "Elim")
	UMaterialInstance* BlueDissolveMaterialInstance;
	
	UPROPERTY(EditAnywhere, Category = "Elim")
	UMaterialInstance* BlueMaterialInstance;
	
	UPROPERTY(EditAnywhere, Category = "Elim")
	UMaterialInstance* OriginalMaterialInstance;
	
	UPROPERTY(EditAnywhere, Category = "Elim")
	UMaterialInstance* OriginalDissolveMaterialInstance;
	
	/**
	 * Elim Effects
	 */

	UPROPERTY(EditAnywhere, Category = "Elim")
	UParticleSystem* ElimBotEffect;

	UPROPERTY(VisibleAnywhere, Category = "Elim")
	UParticleSystemComponent* ElimBotComponent;
	
	UPROPERTY(EditAnywhere, Category = "Elim")
	class USoundCue* ElimBotSound;

	UPROPERTY(EditAnywhere)
	class UNiagaraSystem* CrownSystem;

	UPROPERTY()
	class UNiagaraComponent* CrownComponent;

	/**
	 * Grenade
	 */

	UPROPERTY(VisibleAnywhere)
	UStaticMeshComponent* AttachedGrenade;

	/**
	 * Default weapon
	 */

	UPROPERTY(EditAnywhere)
	TSubclassOf<AWeapon> DefaultWeapon;

public:

	/**
	 * Server-side rewind를 위한 히트 박스
	 */

	UPROPERTY(EditAnywhere)
	class UBoxComponent* head;

	UPROPERTY(EditAnywhere)
	UBoxComponent* pelvis;

	UPROPERTY(EditAnywhere)
	UBoxComponent* spine_02;

	UPROPERTY(EditAnywhere)
	UBoxComponent* spine_03;

	UPROPERTY(EditAnywhere)
	UBoxComponent* upperarm_l;

	UPROPERTY(EditAnywhere)
	UBoxComponent* upperarm_r;

	UPROPERTY(EditAnywhere)
	UBoxComponent* lowerarm_l;

	UPROPERTY(EditAnywhere)
	UBoxComponent* lowerarm_r;

	UPROPERTY(EditAnywhere)
	UBoxComponent* hand_l;

	UPROPERTY(EditAnywhere)
	UBoxComponent* hand_r;

	UPROPERTY(EditAnywhere)
	UBoxComponent* backpack;

	UPROPERTY(EditAnywhere)
	UBoxComponent* blanket;

	UPROPERTY(EditAnywhere)
	UBoxComponent* thigh_l;

	UPROPERTY(EditAnywhere)
	UBoxComponent* thigh_r;

	UPROPERTY(EditAnywhere)
	UBoxComponent* calf_l;

	UPROPERTY(EditAnywhere)
	UBoxComponent* calf_r;

	UPROPERTY(EditAnywhere)
	UBoxComponent* foot_l;

	UPROPERTY(EditAnywhere)
	UBoxComponent* foot_r;

	UPROPERTY()
	TMap<FName, UBoxComponent*> HitCollisionBoxes;
	
public:
	// Getter, Setter
	void SetOverlappingWeapon(AWeapon* Weapon, bool bOverlapped);
	bool IsWeaponEquipped();
	bool IsAiming();
	FORCEINLINE float GetAO_Yaw() const { return AO_Yaw; }
	FORCEINLINE float GetAO_Pitch() const { return AO_Pitch; }
	AWeapon* GetEquippedWeapon();
	FORCEINLINE ETurningInPlace GetTurningInPlace() const { return TurningInPlace; }
	FVector GetHitTarget() const;
	FORCEINLINE UCameraComponent* GetFollowCamera() const { return FollowCamera; }
	FORCEINLINE bool ShouldRotateRootBone() const { return bRotateRootBone; }
	FORCEINLINE bool IsElimmed() const { return bElimmed; }
	FORCEINLINE float GetHealth() const { return Health; }
	FORCEINLINE void SetHealth(float Amount) { Health = Amount; }
	FORCEINLINE float GetMaxHealth() const { return MaxHealth; }
	ECombatState GetCombatState() const;
	FORCEINLINE UAnimMontage* GetReloadMontage() const { return ReloadMontage; }
	FORCEINLINE UCombatComponent* GetCombat() const { return Combat; }
	FORCEINLINE UStaticMeshComponent* GetAttachedGrenade() const { return AttachedGrenade; }
	FORCEINLINE UBuffComponent* GetBuff() const { return Buff; }
	FORCEINLINE float GetShield() const { return Shield; }
	FORCEINLINE void SetShield(float Amount) { Shield = Amount; }
	FORCEINLINE float GetMaxShield() const { return MaxShield; }
	bool IsLocallyReloading();
	FORCEINLINE ULagCompensationComponent* GetLagCompensation() const { return LagCompensation; }
	FORCEINLINE bool IsHoldingTheFlag() const;
	
	float GetCameraFOV() const;
	virtual ETeam GetTeam() override;
	void SetHoldingTheFlag(bool bHolding);
};
