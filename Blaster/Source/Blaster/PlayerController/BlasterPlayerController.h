#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerController.h"
#include "BlasterPlayerController.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FHighPingDelegate, bool, bPingTooHigh);

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
	void SetHUDShield(float Shield, float MaxShield);
	void SetHUDScore(float Score);
	void SetHUDDefeats(int32 Defeats);
	void SetHUDWeaponAmmo(int32 Ammo);
	void SetHUDCarriedAmmo(int32 Ammo);
	void SetHUDMatchCountdown(int32 CountdownTime);
	void SetHUDAnnouncementCountdown(int32 CountdownTime);
	void SetHUDGrenades(int32 Grenades);
	virtual void OnPossess(APawn* InPawn) override;
	void HideTeamScores();
	void InitTeamScores();
	void SetHUDRedTeamScore(int32 RedScore);
	void SetHUDBlueTeamScore(int32 BlueScore);

	virtual float GetServerTime();
	// 플레이어 접속 시 호출
	virtual void ReceivedPlayer() override;
	void OnMatchStateSet(FName State, bool bTeamsMatch);
	void HandleWidgetState();
	void HandleMatchHasStarted();
	void HandleCooldown();

	// 클라이언트에서 서버까지 걸리는 시간
	float SingleTripTime;

	FHighPingDelegate HighPingDelegate;

	// 킬로그 전파
	void BroadcastElim(APlayerState* Attacker, APlayerState* Victim);

	UPROPERTY(ReplicatedUsing = OnRep_ShowTeamScores)
	bool bShowTeamScores;

protected:
	virtual void BeginPlay() override;
	
	void SetHUDTime();

	// 초기화 타이밍으로 인해 생기는 각종 문제를 해결하기 위해 반복적으로 호출되는 함수, 여기선 CharacterOverlay의 초기화 상태를 점검한다
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
	void ClientJoinMidgame(FName StateOfMatch, bool bTeamsMatch, float Starting, float Warmup, float Match, float Cooldown);

	void HighPingWarning();
	void StopHighPingWarning();
	void CheckPing(float DeltaTime);

	// 킬로그
	UFUNCTION(Client, Reliable)
	void ClientElimAnnouncement(APlayerState* Attacker, APlayerState* Victim);

	UFUNCTION()
	void OnRep_ShowTeamScores();

	// 매치 종료 시 화면에 나올 문구를 구하는 함수
	FString GetMatchEndText(const TArray<class ABlasterPlayerState*>& Players);
	FString GetTeamsMatchEndText(const class ABlasterGameState* BlasterGameState);
	
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

	float HUDHealth;
	float HUDMaxHealth;
	bool bInitializeHealth = false;
	float HUDShield;
	float HUDMaxShield;
	bool bInitializeShield = false;
	float HUDScore;
	bool bInitializeScore = false;
	int32 HUDDefeats;
	bool bInitializeDefeats = false;
	int32 HUDGrenades;
	float HUDCarriedAmmo;
	bool bInitializeCarriedAmmo = false;
	float HUDWeaponAmmo;
	bool bInitializeWeaponAmmo = false;

	float HighPingRunningTime = 0.f;

	UPROPERTY(EditAnywhere)
	float HighPingDuration = 5.f;

	float PingAnimationRunningTime = 0.f;

	UPROPERTY(EditAnywhere)
	float CheckPingFrequency = 20.f;

	UFUNCTION(Server, Reliable)
	void ServerReportPingStatus(bool bHighPing);

	float HighPingThreshold = 200.f;

	/**
	 * 플레이어 인풋 관련 세팅
	 */

protected:
	void InitDefaultSettings();
	virtual void SetupInputComponent() override;

private:
	UPROPERTY(EditAnywhere, Category = "Input")
	TObjectPtr<class UInputMappingContext> BlasterContext;

	UPROPERTY(EditAnywhere, Category = "Input")
	TObjectPtr<class UInputAction> MoveAction;
	
	UPROPERTY(EditAnywhere, Category = "Input")
	TObjectPtr<UInputAction> JumpAction;

	UPROPERTY(EditAnywhere, Category = "Input")
	TObjectPtr<UInputAction> LookAction;

	UPROPERTY(EditAnywhere, Category = "Input")
	TObjectPtr<UInputAction> EquipAction;

	UPROPERTY(EditAnywhere, Category = "Input")
	TObjectPtr<UInputAction> SwapAction;
	
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

	auto Move(const FInputActionValue& InputActionValue) -> void;
	void Look(const FInputActionValue& InputActionValue);
	void Jump();
	void EquipButtonPressed();
	void SwapButtonPressed();
	void CrouchButtonPressed();
	void AimButtonPressed();
	void AimButtonReleased();
	void FireButtonPressed();
	void FireButtonReleased();
	void ReloadButtonPressed();
	void ThrowGrenadeButtonPressed();

	/**
	 * 시스템
	*/
	
	UPROPERTY(EditAnywhere, Category = "Input")
	TObjectPtr<UInputAction> QuitAction;
	
	UPROPERTY(EditAnywhere, Category = "Input")
	TObjectPtr<UInputAction> ChatAction;
	
	void QuitButtonPressed();
	void ChatButtonPressed();

	UPROPERTY(EditAnywhere, Category = "HUD")
	TSubclassOf<UUserWidget> ReturnToMainMenuWidget;

	UPROPERTY()
	TObjectPtr<class UReturnToMainMenu> ReturnToMainMenu;

	bool bReturnToMainMenuOpen = false;
	
public:
	UPROPERTY(Replicated)
	bool bDisableGameplay = false;

	UFUNCTION(Server, Reliable)
	void ServerChat(const FText& TextToChat);

	UFUNCTION(Client, Reliable)
	void ClientChat(const FText& TextToChat);

	void SetInputGameOnly();
};
