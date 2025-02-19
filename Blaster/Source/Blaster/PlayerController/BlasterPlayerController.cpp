#include "BlasterPlayerController.h"
#include "EnhancedInputSubsystems.h"
#include "EnhancedInputComponent.h"
#include "Blaster/Character/BlasterCharacter.h"
#include "Blaster/HUD/BlasterHUD.h"
#include "Blaster/HUD/CharacterOverlay.h"

ABlasterPlayerController::ABlasterPlayerController()
{
	bReplicates = true;
}

void ABlasterPlayerController::BeginPlay()
{
	Super::BeginPlay();

	if (!IsLocalController())
	{
		return;
	}

	InitDefaultSettings();

	BlasterHUD = Cast<ABlasterHUD>(GetHUD());
}

void ABlasterPlayerController::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	// 클라이언트에서 시간을 계속 계산하며 HUD에 표시
	SetHUDTime();
	CheckTimeSync(DeltaTime);
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

void ABlasterPlayerController::SetHUDTime()
{
	// 매치 시간에서 유추한 서버 시간을 빼는 것으로 남은 시간 계산
	uint32 SecondsLeft = FMath::CeilToInt(MatchTime - GetServerTime());

	// 실질적으로 시간이 바뀌었을 때만 업데이트를 호출해 최적화
	if (CountdownInt != SecondsLeft)
	{
		SetHUDMatchCountdown(MatchTime - GetServerTime());
		CountdownInt = SecondsLeft;
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
	
	if (bHUDValid)
	{
		const float HealthPercent = Health / MaxHealth;
		BlasterHUD->CharacterOverlay->UpdateHealthBar(HealthPercent);
		FString HealthText = FString::Printf(TEXT("%d/%d"), FMath::CeilToInt(Health), FMath::CeilToInt(MaxHealth));
		BlasterHUD->CharacterOverlay->UpdateHealthText(HealthText);
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
		FString ScoreText = FString::Printf(TEXT("%d"), FMath::FloorToInt(Score));
		BlasterHUD->CharacterOverlay->UpdateScoreAmount(ScoreText);
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
		FString DefeatsText = FString::Printf(TEXT("%d"), Defeats);
		BlasterHUD->CharacterOverlay->UpdateDefeatsAmount(DefeatsText);
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
		FString AmmoText = FString::Printf(TEXT("%d"), Ammo);
		BlasterHUD->CharacterOverlay->UpdateWeaponAmmoAmount(AmmoText);
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
		FString AmmoText = FString::Printf(TEXT("%d"), Ammo);
		BlasterHUD->CharacterOverlay->UpdateCarriedAmmoAmount(AmmoText);
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
		int32 Minutes = FMath::FloorToInt(CountdownTime / 60.f);
		int32 Seconds = CountdownTime - Minutes * 60;
		
		FString CountdownText = FString::Printf(TEXT("%02d : %02d"), Minutes, Seconds);
		BlasterHUD->CharacterOverlay->UpdateMatchCountdownText(CountdownText);
	}
}

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
	EnhancedInputComponent->BindAction(CrouchAction, ETriggerEvent::Started, this, &ThisClass::CrouchButtonPressed);
	EnhancedInputComponent->BindAction(AimAction, ETriggerEvent::Started, this, &ThisClass::AimButtonPressed);
	EnhancedInputComponent->BindAction(AimAction, ETriggerEvent::Completed, this, &ThisClass::AimButtonReleased);
	EnhancedInputComponent->BindAction(FireAction, ETriggerEvent::Started, this, &ThisClass::FireButtonPressed);
	EnhancedInputComponent->BindAction(FireAction, ETriggerEvent::Completed, this, &ThisClass::FireButtonReleased);
	EnhancedInputComponent->BindAction(ReloadAction, ETriggerEvent::Started, this, &ThisClass::ReloadButtonPressed);
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

	AddYawInput(LookAxisVector.X);
	AddPitchInput(-LookAxisVector.Y);
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
	if (ACharacter* ControlledPawn = GetCharacter())
	{
		if (ABlasterCharacter* BlasterCharacter = Cast<ABlasterCharacter>(ControlledPawn))
		{
			BlasterCharacter->EquipButtonPressed();
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
	if (ACharacter* ControlledPawn = GetCharacter())
	{
		if (ABlasterCharacter* BlasterCharacter = Cast<ABlasterCharacter>(ControlledPawn))
		{
			BlasterCharacter->ReloadButtonPressed();
		}
	}
}
