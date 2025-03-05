#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerController.h"
#include "BlasterPlayerController.generated.h"

class UInputMappingContext;
class UInputAction;
struct FInputActionValue;

UCLASS()
class BLASTER_API ABlasterPlayerController : public APlayerController
{
	GENERATED_BODY()

public:
	ABlasterPlayerController();
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
	virtual void Tick(float DeltaTime) override;
	void SetHUDHealth(float Health, float MaxHealth);
	void SetHUDScore(float Score);
	void SetHUDDefeats(int32 Defeats);
	void SetHUDWeaponAmmo(int32 Ammo);
	void SetHUDCarriedAmmo(int32 Ammo);
	void SetHUDMatchCountdown(int32 CountdownTime);
	void SetHUDAnnouncementCountdown(int32 CountdownTime);
	void SetHUDGrenades(int32 Grenades);
	virtual void OnPossess(APawn* InPawn) override;

	virtual float GetServerTime();
	// 플레이어 접속 시 호출
	virtual void ReceivedPlayer() override;
	void OnMatchStateSet(FName State);
	void HandleWidgetState();
	void HandleMatchHasStarted();
	void HandleCooldown();

protected:
	virtual void BeginPlay() override;
	
	void SetHUDTime();

	// nullptr 문제를 해소하기 위해 호출하는 함수.
	void PollInit();

	/**
	 * 서버와 클라이언트간 매치 카운트다운 싱크
	 */

	UFUNCTION(Server, Reliable)
	void ServerRequestServerTime(float TimeOfClientRequest);

	UFUNCTION(Client, Reliable)
	void ClientReportServerTime(float TimeOfClientRequest, float TimeServerReceivedClientRequest);

	float ClientServerDelta = 0.f;

	// n초 주기로 매치 카운트다운 싱크 업데이트
	UPROPERTY(EditAnywhere, Category = "Time")
	float TimeSyncFrequency = 5.f;

	// 위 주기를 세기 위한 또 다른 float
	float TimeSyncRunningTime = 0.f;

	// 매치 카운트다운 싱크 업데이트
	void CheckTimeSync(float DeltaTime);

	// 게임 시작 시 서버에게 현재 매치 상태 정보 요청
	UFUNCTION(Server, Reliable)
	void ServerCheckMatchState();

	UFUNCTION(Client, Reliable)
	void ClientJoinMidgame(FName StateOfMatch, float Starting, float Warmup, float Match, float Cooldown);
	
private:
	UPROPERTY()
	class ABlasterHUD* BlasterHUD;

	UPROPERTY()
	class ABlasterGameMode* BlasterGameMode;

	float LevelStartingTime = 0.f;
	float WarmupTime = 0.f;
	float MatchTime = 0.f;
	float CooldownTime = 0.f;
	uint32 CountdownInt;

	UPROPERTY(ReplicatedUsing=OnRep_MatchState)
	FName MatchState;

	UFUNCTION()
	void OnRep_MatchState();

	UPROPERTY()
	class UCharacterOverlay* CharacterOverlay;

	bool bInitializeCharacterOverlay = false;

	float HUDHealth;
	float HUDMaxHealth;
	float HUDScore;
	int32 HUDDefeats;
	int32 HUDGrenades;

	/**
	 * 플레이어 인풋 관련 세팅
	 */

protected:
	void InitDefaultSettings();
	virtual void SetupInputComponent() override;

private:
	UPROPERTY(EditAnywhere, Category = "Input")
	TObjectPtr<UInputMappingContext> BlasterContext;

	UPROPERTY(EditAnywhere, Category = "Input")
	TObjectPtr<UInputAction> MoveAction;
	
	UPROPERTY(EditAnywhere, Category = "Input")
	TObjectPtr<UInputAction> JumpAction;

	UPROPERTY(EditAnywhere, Category = "Input")
	TObjectPtr<UInputAction> LookAction;

	UPROPERTY(EditAnywhere, Category = "Input")
	TObjectPtr<UInputAction> EquipAction;
	
	UPROPERTY(EditAnywhere, Category = "Input")
	TObjectPtr<UInputAction> CrouchAction;
	
	UPROPERTY(EditAnywhere, Category = "Input")
	TObjectPtr<UInputAction> AimAction;
	
	UPROPERTY(EditAnywhere, Category = "Input")
	TObjectPtr<UInputAction> FireAction;
	
	UPROPERTY(EditAnywhere, Category = "Input")
	TObjectPtr<UInputAction> ReloadAction;
	
	UPROPERTY(EditAnywhere, Category = "Input")
	TObjectPtr<UInputAction> ThrowGrenadeAction;

	void Move(const FInputActionValue& InputActionValue);
	void Look(const FInputActionValue& InputActionValue);
	void Jump();
	void StopJumping();
	void EquipButtonPressed();
	void CrouchButtonPressed();
	void AimButtonPressed();
	void AimButtonReleased();
	void FireButtonPressed();
	void FireButtonReleased();
	void ReloadButtonPressed();
	void ThrowGrenadeButtonPressed();
	
public:
	UPROPERTY(Replicated)
	bool bDisableGameplay = false;
};
