#include "BlasterCharacter.h"

#include "Camera/CameraComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/SpringArmComponent.h"

#include "GameFramework/PlayerState.h"
#include "Components/WidgetComponent.h"
#include "Blaster/HUD/OverheadWidget.h"

ABlasterCharacter::ABlasterCharacter()
{
	PrimaryActorTick.bCanEverTick = true;

	CameraBoom = CreateDefaultSubobject<USpringArmComponent>(TEXT("CameraBoom"));
	CameraBoom->SetupAttachment(GetMesh());
	CameraBoom->TargetArmLength = 600.0f;
	CameraBoom->bUsePawnControlRotation = true;
	
	FollowCamera = CreateDefaultSubobject<UCameraComponent>(TEXT("FollowCamera"));
	FollowCamera->SetupAttachment(CameraBoom, USpringArmComponent::SocketName);
	FollowCamera->bUsePawnControlRotation = false;

	bUseControllerRotationYaw = false;
	GetCharacterMovement()->bOrientRotationToMovement = true;

	OverheadWidget = CreateDefaultSubobject<UWidgetComponent>(TEXT("OverheadWidget"));
	OverheadWidget->SetupAttachment(RootComponent);
}

void ABlasterCharacter::BeginPlay()
{
	Super::BeginPlay();
}

void ABlasterCharacter::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

}

void ABlasterCharacter::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);
}

void ABlasterCharacter::UpdatePlayerName()
{
	if (OverheadWidget)
	{
		OverheadWidget->InitWidget();

		if (UOverheadWidget* Widget = Cast<UOverheadWidget>(OverheadWidget->GetUserWidgetObject()))
		{
			Widget->SetPlayerName(GetPlayerState()->GetPlayerName());
		}
	}
}

void ABlasterCharacter::PossessedBy(AController* NewController)
{
	// Server Update
	Super::PossessedBy(NewController);

	UpdatePlayerName();
}

void ABlasterCharacter::OnRep_PlayerState()
{
	// Client Update
	Super::OnRep_PlayerState();

	UpdatePlayerName();
}

