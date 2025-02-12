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

void ABlasterPlayerController::SetHUDHealth(float Health, float MaxHealth)
{
	BlasterHUD = BlasterHUD == nullptr ? Cast<ABlasterHUD>(GetHUD()) : BlasterHUD;

	bool bHUDValid = BlasterHUD
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
		ABlasterCharacter* BlasterCharacter = Cast<ABlasterCharacter>(ControlledPawn);
		if (BlasterCharacter)
		{
			BlasterCharacter->EquipButtonPressed();
		}
	}
}

void ABlasterPlayerController::CrouchButtonPressed()
{
	if (ACharacter* ControlledPawn = GetCharacter())
	{
		ABlasterCharacter* BlasterCharacter = Cast<ABlasterCharacter>(ControlledPawn);
		if (BlasterCharacter)
		{
			BlasterCharacter->CrouchButtonPressed();
		}
	}
}

void ABlasterPlayerController::AimButtonPressed()
{
	if (ACharacter* ControlledPawn = GetCharacter())
	{
		ABlasterCharacter* BlasterCharacter = Cast<ABlasterCharacter>(ControlledPawn);
		if (BlasterCharacter)
		{
			BlasterCharacter->AimButtonPressed();
		}
	}
}

void ABlasterPlayerController::AimButtonReleased()
{
	if (ACharacter* ControlledPawn = GetCharacter())
	{
		ABlasterCharacter* BlasterCharacter = Cast<ABlasterCharacter>(ControlledPawn);
		if (BlasterCharacter)
		{
			BlasterCharacter->AimButtonReleased();
		}
	}
}

void ABlasterPlayerController::FireButtonPressed()
{
	if (ACharacter* ControlledPawn = GetCharacter())
	{
		ABlasterCharacter* BlasterCharacter = Cast<ABlasterCharacter>(ControlledPawn);
		if (BlasterCharacter)
		{
			BlasterCharacter->FireButtonPressed();
		}
	}
}

void ABlasterPlayerController::FireButtonReleased()
{
	if (ACharacter* ControlledPawn = GetCharacter())
	{
		ABlasterCharacter* BlasterCharacter = Cast<ABlasterCharacter>(ControlledPawn);
		if (BlasterCharacter)
		{
			BlasterCharacter->FireButtonReleased();
		}
	}
}
