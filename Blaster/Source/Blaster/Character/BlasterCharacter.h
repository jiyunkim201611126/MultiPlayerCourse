#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "BlasterCharacter.generated.h"

class UInputMappingContext;

UCLASS()
class BLASTER_API ABlasterCharacter : public ACharacter
{
	GENERATED_BODY()

public:
	ABlasterCharacter();
	virtual void Tick(float DeltaTime) override;
	virtual void SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent) override;
	virtual void PostInitializeComponents() override;
	virtual void GetLifetimeReplicatedProps(TArray<class FLifetimeProperty>& OutLifetimeProps) const override;

	void UpdatePlayerName();

	virtual void PossessedBy(AController* NewController) override;
	virtual void OnRep_PlayerState() override;

	void EquipButtonPressed();
	void CrouchButtonPressed();
	void AimButtonPressed();
	void AimButtonReleased();
	
private:
	UPROPERTY(VisibleAnywhere, Category = "Camera")
	class USpringArmComponent* CameraBoom;

	UPROPERTY(VisibleAnywhere, Category = "Camera")
	class UCameraComponent* FollowCamera;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, meta = (AllowPrivateAccess = "true"))
	class UWidgetComponent* OverheadWidget;

	// Overlap될 때 변수 자체는 서버와 클라이언트 모두가 변경되지만, 콜백 함수는 겹친 클라이언트의 인스턴스에서만 호출.
	UPROPERTY(ReplicatedUsing = OnRep_OverlappingWeapon)
	class AWeapon* OverlappingWeapon;

	// Overlap될 때 클라이언트가 호출하는 콜백 함수
	UFUNCTION()
	void OnRep_OverlappingWeapon(AWeapon* LastWeapon);

	UPROPERTY(VisibleAnywhere)
	class UCombatComponent* Combat;

	// 클라이언트가 서버에 요청하는 함수, 패킷 손실돼도 다시 전송해 호출을 반드시 보장함.
	UFUNCTION(Server, Reliable)
	void ServerEquipButtonPressed();
	
	float AO_Yaw;
	float AO_Pitch;
	FRotator StartingAimRotation;

protected:
	virtual void BeginPlay() override;

	void AimOffset(float DeltaTime);
	
public:
	void SetOverlappingWeapon(AWeapon* Weapon);
	bool IsWeaponEquipped();
	bool IsAiming();
	FORCEINLINE float GetAO_Yaw() const { return AO_Yaw; }
	FORCEINLINE float GetAO_Pitch() const { return AO_Pitch; }
	AWeapon* GetEquippedWeapon();
};
