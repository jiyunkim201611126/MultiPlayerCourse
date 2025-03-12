#include "BlasterPlayerController.h"
#include "EnhancedInputSubsystems.h"
#include "EnhancedInputComponent.h"
#include "Blaster/BlasterComponents/CombatComponent.h"
#include "Blaster/Character/BlasterCharacter.h"
#include "Blaster/HUD/BlasterHUD.h"
#include "Blaster/HUD/CharacterOverlay.h"
#include "Net/UnrealNetwork.h"
#include "Blaster/GameMode/BlasterGameMode.h"
#include "Blaster/HUD/Announcement.h"
#include "Components/TextBlock.h"
#include "Kismet/GameplayStatics.h"
#include "Blaster/GameState/BlasterGameState.h"
#include "Blaster/PlayerState/BlasterPlayerState.h"

ABlasterPlayerController::ABlasterPlayerController()
{
	bReplicates = true;
}

void ABlasterPlayerController::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(ABlasterPlayerController, MatchState);
	DOREPLIFETIME(ABlasterPlayerController, bDisableGameplay);
}

void ABlasterPlayerController::BeginPlay()
{
	Super::BeginPlay();
	
	// 게임 시작 시 우선 현재 매치 상태 정보 요청
	BlasterHUD = Cast<ABlasterHUD>(GetHUD());
	ServerCheckMatchState();
	
	if (IsLocalController())
	{
		InitDefaultSettings();
	}
}

void ABlasterPlayerController::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	// 클라이언트에서 시간을 계속 계산하며 HUD에 표시
	SetHUDTime();
	CheckTimeSync(DeltaTime);
	
	PollInit();
}

void ABlasterPlayerController::CheckTimeSync(float DeltaTime)
{
	// 쓰로틀링 걸어서 n초 주기로 서버에 패킷 보냄
	TimeSyncRunningTime += DeltaTime;
	if (IsLocalController() && TimeSyncRunningTime >= TimeSyncFrequency)
	{
		// 서버에게 현재 시간을 보냄
		ServerRequestServerTime(GetWorld()->GetTimeSeconds());
		TimeSyncRunningTime = 0.f;
	}
}

void ABlasterPlayerController::ServerCheckMatchState_Implementation()
{
	// 서버는 게임 모드에서 관련 정보 가져와 초기화
	ABlasterGameMode* GameMode = Cast<ABlasterGameMode>(UGameplayStatics::GetGameMode(this));
	if (GameMode)
	{
		// 게임 시작 직후 필요한 정보들
		MatchState = GameMode->GetMatchState();
		LevelStartingTime = GameMode->LevelStartingTime;
		WarmupTime = GameMode->WarmupTime;
		MatchTime = GameMode->MatchTime;
		CooldownTime = GameMode->CooldownTime;

		// 위 정보를 클라이언트에게 전송. 단, 플레이어 컨트롤러는 서버도 하나 소유하기 때문에, 아래 함수는 서버도 호출함.
		ClientJoinMidgame(MatchState, LevelStartingTime, WarmupTime, MatchTime, CooldownTime);
	}
}

void ABlasterPlayerController::ClientJoinMidgame_Implementation(FName StateOfMatch, float Starting, float Warmup, float Match, float Cooldown)
{
	// 서버는 이미 초기화했으므로 클라이언트만 걸러서 초기화해줌
	if (IsLocalController())
	{
		MatchState = StateOfMatch;
		LevelStartingTime = Starting;
		WarmupTime = Warmup;
		MatchTime = Match;
		CooldownTime = Cooldown;
	}
	
	// MatchState의 변경 사항에 따른 동작은 서버와 클라이언트 모두가 실행
	OnMatchStateSet(MatchState);
}

void ABlasterPlayerController::PollInit()
{
	if (CharacterOverlay == nullptr)
	{
		if (BlasterHUD && BlasterHUD->CharacterOverlay)
		{
			CharacterOverlay = BlasterHUD->CharacterOverlay;
			if (CharacterOverlay)
			{
				if (bInitializeHealth) SetHUDHealth(HUDHealth, HUDMaxHealth);
				if (bInitializeShield) SetHUDShield(HUDShield, HUDMaxShield);
				if (bInitializeScore) SetHUDScore(HUDScore);
				if (bInitializeDefeats) SetHUDDefeats(HUDDefeats);
				if (bInitializeCarriedAmmo) SetHUDCarriedAmmo(HUDCarriedAmmo);
				if (bInitializeWeaponAmmo) SetHUDWeaponAmmo(HUDWeaponAmmo);
				ABlasterCharacter* BlasterCharacter = Cast<ABlasterCharacter>(GetPawn());
				if (BlasterCharacter && BlasterCharacter->GetCombat())
				{
					if (bInitializeGrenades) SetHUDGrenades(BlasterCharacter->GetCombat()->GetGrenades());
				}
			}
		}
	}
}

void ABlasterPlayerController::ServerRequestServerTime_Implementation(float TimeOfClientRequest)
{
	// 현재 서버 시간
	float ServerTimeOfReceipt = GetWorld()->GetTimeSeconds();
	// 현재 서버의 시간과 함께 클라이언트에게 받은 시간은 그대로 돌려보냄
	ClientReportServerTime(TimeOfClientRequest, ServerTimeOfReceipt);
}

void ABlasterPlayerController::ClientReportServerTime_Implementation(float TimeOfClientRequest,
	float TimeServerReceivedClientRequest)
{
	// 현재 시간에서 처음 서버에게 요청한 시간을 빼는 것으로, 패킷이 돌아오는 데까지 걸린 시간을 계산
	float RoundTripTime = GetWorld()->GetTimeSeconds() - TimeOfClientRequest;
	// 위 시간의 절반을 서버가 패킷을 보내줄 때의 시간에 더해서 현재 서버의 시간을 유추
	float CurrentServerTime = TimeServerReceivedClientRequest + (0.5f * RoundTripTime);
	// 유추한 서버의 현재 시간과 클라이언트의 시간의 차이
	ClientServerDelta = CurrentServerTime - GetWorld()->GetTimeSeconds();
}

float ABlasterPlayerController::GetServerTime()
{
	if (HasAuthority())
	{
		// 서버는 자신의 시간을 사용하면 됨
		return GetWorld()->GetTimeSeconds();
	}
	else
	{
		// 클라이언트는 서버의 시간을 유추함
		return GetWorld()->GetTimeSeconds() + ClientServerDelta;
	}
}

void ABlasterPlayerController::ReceivedPlayer()
{
	Super::ReceivedPlayer();
	if (IsLocalController())
	{
		ServerRequestServerTime(GetWorld()->GetTimeSeconds());
	}
}

void ABlasterPlayerController::OnPossess(APawn* InPawn)
{
	Super::OnPossess(InPawn);

	bDisableGameplay = false;

	if (ABlasterCharacter* BlasterCharacter = Cast<ABlasterCharacter>(InPawn))
	{
		SetHUDHealth(BlasterCharacter->GetHealth(), BlasterCharacter->GetMaxHealth());
	}
}

void ABlasterPlayerController::SetHUDHealth(float Health, float MaxHealth)
{
	BlasterHUD = BlasterHUD == nullptr ? Cast<ABlasterHUD>(GetHUD()) : BlasterHUD;

	const bool bHUDValid = BlasterHUD
		&& BlasterHUD->CharacterOverlay
		&& BlasterHUD->CharacterOverlay->HealthBar
		&& BlasterHUD->CharacterOverlay->HealthTextBlock;
	
	const float HealthPercent = Health / MaxHealth;
	float VisibleHealth;
	if (0.f < Health && Health <= 1.f)
	{
		VisibleHealth = 1.f;
	}
	else if (Health <= 0.f)
	{
		VisibleHealth = 0.f;
	}
	else
	{
		VisibleHealth = Health;
	}
	VisibleHealth = FMath::Clamp(VisibleHealth, 0.f, MaxHealth);
	
	if (bHUDValid)
	{
		HUDHealth = VisibleHealth;
		HUDMaxHealth = MaxHealth;
		BlasterHUD->CharacterOverlay->UpdateHealthBar(HealthPercent);
		FString HealthText = FString::Printf(TEXT("%d/%d"), FMath::RoundToInt(VisibleHealth), FMath::RoundToInt(MaxHealth));
		BlasterHUD->CharacterOverlay->UpdateHealthText(HealthText);
	}
	else
	{
		bInitializeHealth = true;
		HUDHealth = VisibleHealth;
		HUDMaxHealth = MaxHealth;
	}
}

void ABlasterPlayerController::SetHUDShield(float Shield, float MaxShield)
{
	BlasterHUD = BlasterHUD == nullptr ? Cast<ABlasterHUD>(GetHUD()) : BlasterHUD;

	const bool bHUDValid = BlasterHUD
		&& BlasterHUD->CharacterOverlay
		&& BlasterHUD->CharacterOverlay->ShieldBar
		&& BlasterHUD->CharacterOverlay->ShieldTextBlock;

	const float ShieldPercent = Shield / MaxShield;
	float VisibleShield;
	if (0.f < Shield && Shield <= 1.f)
	{
		VisibleShield = 1.f;
	}
	else if (Shield <= 0.f)
	{
		VisibleShield = 0.f;
	}
	else
	{
		VisibleShield = Shield;
	}
	VisibleShield = FMath::Clamp(VisibleShield, 0.f, MaxShield);
	
	if (bHUDValid)
	{
		HUDShield = VisibleShield;
		HUDMaxShield = MaxShield;
		BlasterHUD->CharacterOverlay->UpdateShieldBar(ShieldPercent);
		FString ShieldText = FString::Printf(TEXT("%d/%d"), FMath::RoundToInt(VisibleShield), FMath::RoundToInt(MaxShield));
		BlasterHUD->CharacterOverlay->UpdateShieldText(ShieldText);
	}
	else
	{
		bInitializeShield = true;
		HUDShield = VisibleShield;
		HUDMaxShield = MaxShield;
	}
}

void ABlasterPlayerController::SetHUDScore(float Score)
{
	BlasterHUD = BlasterHUD == nullptr ? Cast<ABlasterHUD>(GetHUD()) : BlasterHUD;

	const bool bHUDValid = BlasterHUD
		&& BlasterHUD->CharacterOverlay
		&& BlasterHUD->CharacterOverlay->ScoreAmount;

	if (bHUDValid)
	{
		HUDScore = Score;
		FString ScoreText = FString::Printf(TEXT("%d"), FMath::RoundToInt(Score));
		BlasterHUD->CharacterOverlay->UpdateScoreAmount(ScoreText);
	}
	else
	{
		bInitializeScore = true;
		HUDScore = Score;
	}
}

void ABlasterPlayerController::SetHUDDefeats(int32 Defeats)
{
	BlasterHUD = BlasterHUD == nullptr ? Cast<ABlasterHUD>(GetHUD()) : BlasterHUD;

	const bool bHUDValid = BlasterHUD
		&& BlasterHUD->CharacterOverlay
		&& BlasterHUD->CharacterOverlay->DefeatsAmount;

	if (bHUDValid)
	{
		HUDDefeats = Defeats;
		FString DefeatsText = FString::Printf(TEXT("%d"), Defeats);
		BlasterHUD->CharacterOverlay->UpdateDefeatsAmount(DefeatsText);
	}
	else
	{
		bInitializeDefeats = true;
		HUDDefeats = Defeats;
	}
}

void ABlasterPlayerController::SetHUDWeaponAmmo(int32 Ammo)
{
	BlasterHUD = BlasterHUD == nullptr ? Cast<ABlasterHUD>(GetHUD()) : BlasterHUD;

	const bool bHUDValid = BlasterHUD
		&& BlasterHUD->CharacterOverlay
		&& BlasterHUD->CharacterOverlay->WeaponAmmoAmount;

	if (bHUDValid)
	{
		HUDWeaponAmmo = Ammo;
		FString AmmoText = FString::Printf(TEXT("%d"), Ammo);
		BlasterHUD->CharacterOverlay->UpdateWeaponAmmoAmount(AmmoText);
	}
	else
	{
		bInitializeWeaponAmmo = true;
		HUDWeaponAmmo = Ammo;
	}
}

void ABlasterPlayerController::SetHUDCarriedAmmo(int32 Ammo)
{
	BlasterHUD = BlasterHUD == nullptr ? Cast<ABlasterHUD>(GetHUD()) : BlasterHUD;

	const bool bHUDValid = BlasterHUD
		&& BlasterHUD->CharacterOverlay
		&& BlasterHUD->CharacterOverlay->CarriedAmmoAmount;

	if (bHUDValid)
	{
		HUDCarriedAmmo = Ammo;
		FString AmmoText = FString::Printf(TEXT("%d"), Ammo);
		BlasterHUD->CharacterOverlay->UpdateCarriedAmmoAmount(AmmoText);
	}
	else
	{
		bInitializeCarriedAmmo = true;
		HUDCarriedAmmo = Ammo;
	}
}

void ABlasterPlayerController::SetHUDTime()
{
	// HUD에 표시해야 하는 시간 계산
	float TimeLeft = 0.f;
	if (MatchState == MatchState::WaitingToStart)
	{
		// WarmupTime에서 게임 시작 후 지난 시간만큼 빼기
		TimeLeft = WarmupTime - GetServerTime() + LevelStartingTime;
	}
	else if (MatchState == MatchState::InProgress)
	{
		// WarmupTime + MatchTime에서 게임 시작 후 지난 시간만큼 빼기
		TimeLeft = WarmupTime + MatchTime - GetServerTime() + LevelStartingTime;
	}
	else if (MatchState == MatchState::Cooldown)
	{
		TimeLeft = WarmupTime + MatchTime + CooldownTime - GetServerTime() + LevelStartingTime;
	}
	
	// 매치 시간에서 유추한 서버 시간을 빼는 것으로 남은 시간 계산
	uint32 SecondsLeft = FMath::FloorToInt(TimeLeft);

	// 실질적으로 시간이 바뀌었을 때만 업데이트를 호출해 최적화
	if (CountdownInt != SecondsLeft)
	{
		if (MatchState == MatchState::WaitingToStart || MatchState == MatchState::Cooldown)
		{
			SetHUDAnnouncementCountdown(TimeLeft);
		}
		if (MatchState == MatchState::InProgress)
		{
			SetHUDMatchCountdown(TimeLeft);
		}
		CountdownInt = SecondsLeft;
	}
}

void ABlasterPlayerController::SetHUDMatchCountdown(int32 CountdownTime)
{
	BlasterHUD = BlasterHUD == nullptr ? Cast<ABlasterHUD>(GetHUD()) : BlasterHUD;

	const bool bHUDValid = BlasterHUD
		&& BlasterHUD->CharacterOverlay
		&& BlasterHUD->CharacterOverlay->MatchCountdownText;

	if (bHUDValid)
	{
		if (CountdownTime < 0.f)
		{
			BlasterHUD->CharacterOverlay->UpdateMatchCountdownText(FString());
			return;
		}
		
		int32 Minutes = FMath::FloorToInt(CountdownTime / 60.f);
		int32 Seconds = CountdownTime - Minutes * 60;
		
		FString CountdownText = FString::Printf(TEXT("%02d : %02d"), Minutes, Seconds);
		BlasterHUD->CharacterOverlay->UpdateMatchCountdownText(CountdownText);

		if (CountdownTime < 30.f)
		{
			BlasterHUD->CharacterOverlay->LerpMatchCountdownTextColor();
		}
	}
}

void ABlasterPlayerController::SetHUDAnnouncementCountdown(int32 CountdownTime)
{
	BlasterHUD = BlasterHUD == nullptr ? Cast<ABlasterHUD>(GetHUD()) : BlasterHUD;

	const bool bHUDValid = BlasterHUD
		&& BlasterHUD->Announcement
		&& BlasterHUD->Announcement->WarmupTimeText;

	if (bHUDValid)
	{
		if (CountdownTime < 0.f)
		{
			BlasterHUD->Announcement->UpdateWarmupTimeText(FString());
			return;
		}
		
		int32 Minutes = FMath::FloorToInt(CountdownTime / 60.f);
		int32 Seconds = CountdownTime - Minutes * 60;
		
		FString CountdownText = FString::Printf(TEXT("%02d : %02d"), Minutes, Seconds);
		BlasterHUD->Announcement->UpdateWarmupTimeText(CountdownText);
	}
}

void ABlasterPlayerController::SetHUDGrenades(int32 Grenades)
{
	BlasterHUD = BlasterHUD == nullptr ? Cast<ABlasterHUD>(GetHUD()) : BlasterHUD;

	const bool bHUDValid = BlasterHUD
		&& BlasterHUD->CharacterOverlay
		&& BlasterHUD->CharacterOverlay->GrenadesText;

	if (bHUDValid)
	{
		FString GrenadeText = FString::Printf(TEXT("%d"), Grenades);
		BlasterHUD->CharacterOverlay->UpdateGrenadesAmount(GrenadeText);
	}
	else
	{
		bInitializeGrenades = true;
		HUDGrenades = Grenades;
	}
}

void ABlasterPlayerController::OnMatchStateSet(FName State)
{
	// 변경된 MatchState를 적용하고, 상태에 따라 HUD를 새로고침
	MatchState = State;

	HandleWidgetState();
}

void ABlasterPlayerController::OnRep_MatchState()
{
	HandleWidgetState();
}

void ABlasterPlayerController::HandleWidgetState()
{
	if (BlasterHUD && MatchState == MatchState::WaitingToStart)
	{
		BlasterHUD->AddAnnouncement();
	}
	
	if (MatchState == MatchState::InProgress)
	{
		HandleMatchHasStarted();
	}
	else if (MatchState == MatchState::Cooldown)
	{
		HandleCooldown();
	}
}

void ABlasterPlayerController::HandleMatchHasStarted()
{
	BlasterHUD = BlasterHUD == nullptr ? Cast<ABlasterHUD>(GetHUD()) : BlasterHUD;
	if (BlasterHUD)
	{
		BlasterHUD->AddCharacterOverlay();
		SetHUDTime();

		if (BlasterHUD->Announcement)
		{
			BlasterHUD->Announcement->SetVisibility(ESlateVisibility::Collapsed);
		}
	}
}

void ABlasterPlayerController::HandleCooldown()
{
	BlasterHUD = BlasterHUD == nullptr ? Cast<ABlasterHUD>(GetHUD()) : BlasterHUD;
	if (BlasterHUD)
	{
		BlasterHUD->CharacterOverlay->RemoveFromParent();

		if (BlasterHUD->Announcement)
		{
			BlasterHUD->Announcement->SetVisibility(ESlateVisibility::Visible);
			FString AnnouncementText("New Match Starts In:");
			BlasterHUD->Announcement->UpdateAnnouncementText(AnnouncementText);

			// 대기 시간 중 직전 라운드 우승자 표시
			ABlasterGameState* BlasterGameState = Cast<ABlasterGameState>(UGameplayStatics::GetGameState(this));
			ABlasterPlayerState* BlasterPlayerState = GetPlayerState<ABlasterPlayerState>();
			if (BlasterGameState && BlasterPlayerState)
			{
				TArray<ABlasterPlayerState*> TopPlayers = BlasterGameState->TopScoringPlayers;
				FString InfoTextString;
				if (TopPlayers.Num() == 0)
				{
					InfoTextString = FString("There is no winner.");
				}
				else if (TopPlayers.Num() == 1 && TopPlayers[0] == BlasterPlayerState)
				{
					InfoTextString = FString("You are the winner!");
				}
				else if (TopPlayers.Num() == 1)
				{
					InfoTextString = FString::Printf(TEXT("Winner: \n%s"), *TopPlayers[0]->GetPlayerName());
				}
				else if (TopPlayers.Num() > 1)
				{
					InfoTextString = FString("Players tied for the win: \n");
					for (const auto TiedPlayer : TopPlayers)
					{
						InfoTextString.Append(FString::Printf(TEXT("%s\n"), *TiedPlayer->GetPlayerName()));
					}
				}
				
				BlasterHUD->Announcement->UpdateInfoText(InfoTextString);
			}
			
			SetHUDTime();
		}
	}

	// 일부 조작 중단, 사격 중이면 사격 중단
	bDisableGameplay = true;
	FireButtonReleased();
}

/**
 * 플레이어 인풋 관련 세팅
 */

void ABlasterPlayerController::InitDefaultSettings()
{
	check(BlasterContext);

	UEnhancedInputLocalPlayerSubsystem* Subsystem = ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(GetLocalPlayer());
	check(Subsystem);
	Subsystem->AddMappingContext(BlasterContext, 0);

	bShowMouseCursor = false;
	DefaultMouseCursor = EMouseCursor::Default;

	FInputModeGameOnly InputModeData;
	InputModeData.SetConsumeCaptureMouseDown(false);
	SetInputMode(InputModeData);
}

void ABlasterPlayerController::SetupInputComponent()
{
	Super::SetupInputComponent();

	UEnhancedInputComponent* EnhancedInputComponent = CastChecked<UEnhancedInputComponent>(InputComponent);

	EnhancedInputComponent->BindAction(MoveAction, ETriggerEvent::Triggered, this, &ThisClass::Move);
	EnhancedInputComponent->BindAction(LookAction, ETriggerEvent::Triggered, this, &ThisClass::Look);
	EnhancedInputComponent->BindAction(JumpAction, ETriggerEvent::Started, this, &ThisClass::Jump);
	EnhancedInputComponent->BindAction(JumpAction, ETriggerEvent::Completed, this, &ThisClass::StopJumping);
	
	EnhancedInputComponent->BindAction(EquipAction, ETriggerEvent::Started, this, &ThisClass::EquipButtonPressed);
	EnhancedInputComponent->BindAction(SwapAction, ETriggerEvent::Started, this, &ThisClass::SwapButtonPressed);
	EnhancedInputComponent->BindAction(CrouchAction, ETriggerEvent::Started, this, &ThisClass::CrouchButtonPressed);
	EnhancedInputComponent->BindAction(AimAction, ETriggerEvent::Started, this, &ThisClass::AimButtonPressed);
	EnhancedInputComponent->BindAction(AimAction, ETriggerEvent::Completed, this, &ThisClass::AimButtonReleased);
	EnhancedInputComponent->BindAction(FireAction, ETriggerEvent::Started, this, &ThisClass::FireButtonPressed);
	EnhancedInputComponent->BindAction(FireAction, ETriggerEvent::Completed, this, &ThisClass::FireButtonReleased);
	EnhancedInputComponent->BindAction(ReloadAction, ETriggerEvent::Started, this, &ThisClass::ReloadButtonPressed);
	EnhancedInputComponent->BindAction(ThrowGrenadeAction, ETriggerEvent::Started, this, &ThisClass::ThrowGrenadeButtonPressed);
}

void ABlasterPlayerController::Move(const FInputActionValue& InputActionValue)
{
	const FVector2d InputAxisVector = InputActionValue.Get<FVector2D>();
	const FRotator Rotation = GetControlRotation();
	const FRotator YawRotation(0.f, Rotation.Yaw, 0.f);

	const FVector ForwardDirection = FRotationMatrix(YawRotation).GetUnitAxis(EAxis::X);
	const FVector RightDirection = FRotationMatrix(Rotation).GetUnitAxis(EAxis::Y);

	if (APawn* ControlledPawn = GetPawn<APawn>())
	{
		ControlledPawn->AddMovementInput(ForwardDirection, InputAxisVector.Y);
		ControlledPawn->AddMovementInput(RightDirection, InputAxisVector.X);
	}
}

void ABlasterPlayerController::Look(const FInputActionValue& InputActionValue)
{
	FVector2D LookAxisVector = InputActionValue.Get<FVector2D>();

	constexpr float DefaultFOV = 90.f;
	float CurrentFOV = PlayerCameraManager ? PlayerCameraManager->GetFOVAngle() : DefaultFOV;

	float SensitivityScale = CurrentFOV / DefaultFOV;

	AddYawInput(LookAxisVector.X * SensitivityScale);
	AddPitchInput(-LookAxisVector.Y * SensitivityScale);
}

void ABlasterPlayerController::Jump()
{	
	if (ACharacter* ControlledPawn = GetCharacter())
	{
		ABlasterCharacter* BlasterCharacter = Cast<ABlasterCharacter>(ControlledPawn);
		BlasterCharacter->Jump();
	}
}

void ABlasterPlayerController::StopJumping()
{
	if (ACharacter* ControlledPawn = GetCharacter())
	{
		ControlledPawn->StopJumping();
	}
}

void ABlasterPlayerController::EquipButtonPressed()
{
	if (bDisableGameplay)
	{
		return;
	}
	
	if (ACharacter* ControlledPawn = GetCharacter())
	{
		if (ABlasterCharacter* BlasterCharacter = Cast<ABlasterCharacter>(ControlledPawn))
		{
			BlasterCharacter->EquipButtonPressed();
		}
	}
}

void ABlasterPlayerController::SwapButtonPressed()
{
	if (ACharacter* ControlledPawn = GetCharacter())
	{
		if (ABlasterCharacter* BlasterCharacter = Cast<ABlasterCharacter>(ControlledPawn))
		{
			BlasterCharacter->SwapButtonPressed();
		}
	}
}

void ABlasterPlayerController::CrouchButtonPressed()
{
	if (ACharacter* ControlledPawn = GetCharacter())
	{
		if (ABlasterCharacter* BlasterCharacter = Cast<ABlasterCharacter>(ControlledPawn))
		{
			BlasterCharacter->CrouchButtonPressed();
		}
	}
}

void ABlasterPlayerController::AimButtonPressed()
{
	if (bDisableGameplay)
	{
		return;
	}
	
	if (ACharacter* ControlledPawn = GetCharacter())
	{
		if (ABlasterCharacter* BlasterCharacter = Cast<ABlasterCharacter>(ControlledPawn))
		{
			BlasterCharacter->AimButtonPressed();
		}
	}
}

void ABlasterPlayerController::AimButtonReleased()
{
	if (ACharacter* ControlledPawn = GetCharacter())
	{
		if (ABlasterCharacter* BlasterCharacter = Cast<ABlasterCharacter>(ControlledPawn))
		{
			BlasterCharacter->AimButtonReleased();
		}
	}
}

void ABlasterPlayerController::FireButtonPressed()
{
	if (bDisableGameplay)
	{
		return;
	}
	
	if (ACharacter* ControlledPawn = GetCharacter())
	{
		if (ABlasterCharacter* BlasterCharacter = Cast<ABlasterCharacter>(ControlledPawn))
		{
			BlasterCharacter->FireButtonPressed();
		}
	}
}

void ABlasterPlayerController::FireButtonReleased()
{
	if (ACharacter* ControlledPawn = GetCharacter())
	{
		if (ABlasterCharacter* BlasterCharacter = Cast<ABlasterCharacter>(ControlledPawn))
		{
			BlasterCharacter->FireButtonReleased();
		}
	}
}

void ABlasterPlayerController::ReloadButtonPressed()
{
	if (bDisableGameplay)
	{
		return;
	}
	
	if (ACharacter* ControlledPawn = GetCharacter())
	{
		if (ABlasterCharacter* BlasterCharacter = Cast<ABlasterCharacter>(ControlledPawn))
		{
			BlasterCharacter->ReloadButtonPressed();
		}
	}
}

void ABlasterPlayerController::ThrowGrenadeButtonPressed()
{
	if (bDisableGameplay)
	{
		return;
	}
	
	if (ACharacter* ControlledPawn = GetCharacter())
	{
		if (ABlasterCharacter* BlasterCharacter = Cast<ABlasterCharacter>(ControlledPawn))
		{
			BlasterCharacter->GrenadeButtonPressed();
		}
	}
}
