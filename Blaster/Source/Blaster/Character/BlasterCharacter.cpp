#include "BlasterCharacter.h"

// Base
#include "Camera/CameraComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/SpringArmComponent.h"
#include "Components/CapsuleComponent.h"
#include "BlasterAnimInstance.h"
#include "Blaster/PlayerController/BlasterPlayerController.h"
#include "Blaster/GameMode/BlasterGameMode.h"
#include "Blaster/PlayerState/BlasterPlayerState.h"

// Replicate
#include "Net/UnrealNetwork.h"

// Show Username
#include "GameFramework/PlayerState.h"
#include "Components/WidgetComponent.h"
#include "Blaster/HUD/OverheadWidget.h"

// Combat
#include "NiagaraComponent.h"
#include "Blaster/Weapon/Weapon.h"
#include "Blaster/BlasterComponents/CombatComponent.h"
#include "Kismet/KismetMathLibrary.h"
#include "Blaster/Blaster.h"
#include "Blaster/Weapon/DamageType/BaseDamageType.h"
#include "Kismet/GameplayStatics.h"
#include "Particles/ParticleSystemComponent.h"
#include "Sound/SoundCue.h"
#include "Blaster/Weapon/WeaponTypes.h"
#include "Blaster/BlasterComponents/BuffComponent.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Components/BoxComponent.h"
#include "Blaster/BlasterComponents/LagCompensationComponent.h"
#include "NiagaraSystem.h"
#include "NiagaraFunctionLibrary.h"
#include "Blaster/GameState/BlasterGameState.h"

ABlasterCharacter::ABlasterCharacter()
{
	PrimaryActorTick.bCanEverTick = true;
	SpawnCollisionHandlingMethod = ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn;
	
	// 서버가 클라이언트의 모습을 보기 전까지 Socket들의 위치가 올바르지 않은 경우를 해결하기 위한 옵션 설정
	GetMesh()->VisibilityBasedAnimTickOption = EVisibilityBasedAnimTickOption::AlwaysTickPoseAndRefreshBones;

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

	Combat = CreateDefaultSubobject<UCombatComponent>(TEXT("CombatComponent"));
	Combat->SetIsReplicated(true);

	Buff = CreateDefaultSubobject<UBuffComponent>(TEXT("BuffComponent"));
	Buff->SetIsReplicated(true);

	LagCompensation = CreateDefaultSubobject<ULagCompensationComponent>(TEXT("LagCompensationComponent"));

	GetCharacterMovement()->NavAgentProps.bCanCrouch = true;
	GetCapsuleComponent()->SetCollisionResponseToChannel(ECC_Camera, ECR_Ignore);
	GetMesh()->SetCollisionObjectType(ECC_SkeletalMesh);
	GetMesh()->SetCollisionResponseToChannel(ECC_Camera, ECR_Ignore);
	GetMesh()->SetCollisionResponseToChannel(ECC_Visibility, ECR_Block);
	GetCharacterMovement()->RotationRate = FRotator(0.f, 0.f, 850.f);

	TurningInPlace = ETurningInPlace::ETIP_NotTurning;

	NetUpdateFrequency = 66.f;
	MinNetUpdateFrequency = 33.f;

	DissolveTimeline = CreateDefaultSubobject<UTimelineComponent>(TEXT("DissolveTimelineComponent"));

	AttachedGrenade = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("AttachedGrenade"));
	AttachedGrenade->SetupAttachment(GetMesh(), FName(TEXT("GrenadeSocket")));
	AttachedGrenade->SetCollisionEnabled(ECollisionEnabled::NoCollision);

	
	head = CreateDefaultSubobject<UBoxComponent>(TEXT("head"));
	head->SetupAttachment(GetMesh(), FName(TEXT("head")));
	HitCollisionBoxes.Add(FName("head"), head);
	
	pelvis = CreateDefaultSubobject<UBoxComponent>(TEXT("pelvis"));
	pelvis->SetupAttachment(GetMesh(), FName(TEXT("pelvis")));
	HitCollisionBoxes.Add(FName("pelvis"), pelvis);
	
	spine_02 = CreateDefaultSubobject<UBoxComponent>(TEXT("spine_02"));
	spine_02->SetupAttachment(GetMesh(), FName(TEXT("spine_02")));
	HitCollisionBoxes.Add(FName("spine_02"), spine_02);
	
	spine_03 = CreateDefaultSubobject<UBoxComponent>(TEXT("spine_03"));
	spine_03->SetupAttachment(GetMesh(), FName(TEXT("spine_03")));
	HitCollisionBoxes.Add(FName("spine_03"), spine_03);
	
	upperarm_l = CreateDefaultSubobject<UBoxComponent>(TEXT("upperarm_l"));
	upperarm_l->SetupAttachment(GetMesh(), FName(TEXT("upperarm_l")));
	HitCollisionBoxes.Add(FName("upperarm_l"), upperarm_l);
	
	upperarm_r = CreateDefaultSubobject<UBoxComponent>(TEXT("upperarm_r"));
	upperarm_r->SetupAttachment(GetMesh(), FName(TEXT("upperarm_r")));
	HitCollisionBoxes.Add(FName("upperarm_r"), upperarm_r);
	
	lowerarm_l = CreateDefaultSubobject<UBoxComponent>(TEXT("lowerarm_l"));
	lowerarm_l->SetupAttachment(GetMesh(), FName(TEXT("lowerarm_l")));
	HitCollisionBoxes.Add(FName("lowerarm_l"), lowerarm_l);
	
	lowerarm_r = CreateDefaultSubobject<UBoxComponent>(TEXT("lowerarm_r"));
	lowerarm_r->SetupAttachment(GetMesh(), FName(TEXT("lowerarm_r")));
	HitCollisionBoxes.Add(FName("lowerarm_r"), lowerarm_r);
	
	hand_l = CreateDefaultSubobject<UBoxComponent>(TEXT("hand_l"));
	hand_l->SetupAttachment(GetMesh(), FName(TEXT("hand_l")));
	HitCollisionBoxes.Add(FName("hand_l"), hand_l);
	
	hand_r = CreateDefaultSubobject<UBoxComponent>(TEXT("hand_r"));
	hand_r->SetupAttachment(GetMesh(), FName(TEXT("hand_r")));
	HitCollisionBoxes.Add(FName("hand_r"), hand_r);
	
	thigh_l = CreateDefaultSubobject<UBoxComponent>(TEXT("thigh_l"));
	thigh_l->SetupAttachment(GetMesh(), FName(TEXT("thigh_l")));
	HitCollisionBoxes.Add(FName("thigh_l"), thigh_l);
	
	thigh_r = CreateDefaultSubobject<UBoxComponent>(TEXT("thigh_r"));
	thigh_r->SetupAttachment(GetMesh(), FName(TEXT("thigh_r")));
	HitCollisionBoxes.Add(FName("thigh_r"), thigh_r);
	
	calf_l = CreateDefaultSubobject<UBoxComponent>(TEXT("calf_l"));
	calf_l->SetupAttachment(GetMesh(), FName(TEXT("calf_l")));
	HitCollisionBoxes.Add(FName("calf_l"), calf_l);
	
	calf_r = CreateDefaultSubobject<UBoxComponent>(TEXT("calf_r"));
	calf_r->SetupAttachment(GetMesh(), FName(TEXT("calf_r")));
	HitCollisionBoxes.Add(FName("calf_r"), calf_r);
	
	foot_l = CreateDefaultSubobject<UBoxComponent>(TEXT("foot_l"));
	foot_l->SetupAttachment(GetMesh(), FName(TEXT("foot_l")));
	HitCollisionBoxes.Add(FName("foot_l"), foot_l);
	
	foot_r = CreateDefaultSubobject<UBoxComponent>(TEXT("foot_r"));
	foot_r->SetupAttachment(GetMesh(), FName(TEXT("foot_r")));
	HitCollisionBoxes.Add(FName("foot_r"), foot_r);

	for (auto Box : HitCollisionBoxes)
	{
		if (Box.Value)
		{
			Box.Value->SetCollisionObjectType(ECC_HitBox);
			Box.Value->SetCollisionResponseToChannels(ECollisionResponse::ECR_Ignore);
			Box.Value->SetCollisionResponseToChannel(ECC_HitBox, ECollisionResponse::ECR_Block);
			Box.Value->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		}
	}
}

void ABlasterCharacter::PostInitializeComponents()
{
	Super::PostInitializeComponents();

	if (Combat)
	{
		Combat->Character = this;
	}
	if (Buff)
	{
		Buff->Character = this;
		Buff->SetInitialSpeeds(GetCharacterMovement()->MaxWalkSpeed, GetCharacterMovement()->MaxWalkSpeedCrouched);
		Buff->SetInitialJumpVelocity(GetCharacterMovement()->JumpZVelocity);
	}
	if (LagCompensation)
	{
		LagCompensation->Character = this;
		if (Controller)
		{
			LagCompensation->Controller = Cast<ABlasterPlayerController>(Controller);
		}
	}
}

void ABlasterCharacter::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);
}

void ABlasterCharacter::GetLifetimeReplicatedProps(TArray<class FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME_CONDITION(ABlasterCharacter, OverlappingWeapons, COND_OwnerOnly);
	DOREPLIFETIME(ABlasterCharacter, Health);
	DOREPLIFETIME(ABlasterCharacter, Shield);
}

void ABlasterCharacter::PossessedBy(AController* NewController)
{
	Super::PossessedBy(NewController);

	// Server Update
	UpdatePlayerName();

	UpdateHUDHealth();
	UpdateHUDShield();
	UpdateHUDAmmo();
}

void ABlasterCharacter::OnRep_PlayerState()
{
	Super::OnRep_PlayerState();

	// Client Update
	UpdatePlayerName();

	UpdateHUDHealth();
	UpdateHUDShield();
	UpdateHUDAmmo();
}

void ABlasterCharacter::UpdatePlayerName() const
{
	if (OverheadWidget)
	{
		OverheadWidget->InitWidget();

		UOverheadWidget* Widget = Cast<UOverheadWidget>(OverheadWidget->GetUserWidgetObject());
		if (Widget)
		{
			Widget->SetPlayerName(GetPlayerState()->GetPlayerName());
			OverheadWidget->SetVisibility(false);
		}
	}
}

void ABlasterCharacter::MulticastGainedTheLead_Implementation()
{
	if (CrownSystem == nullptr) return;
	if (CrownComponent == nullptr)
	{
		CrownComponent = UNiagaraFunctionLibrary::SpawnSystemAttached(
			CrownSystem,
			GetCapsuleComponent(),
			FName(),
			GetActorLocation() + FVector(0.f, 0.f, 110.f),
			GetActorRotation(),
			EAttachLocation::KeepWorldPosition,
			false
			);
	}
	if (CrownComponent)
	{
		CrownComponent->Activate();
	}
}

void ABlasterCharacter::MulticastLostTheLead_Implementation()
{
	if (CrownComponent)
	{
		CrownComponent->DestroyComponent();
	}
}

void ABlasterCharacter::BeginPlay()
{
	Super::BeginPlay();
                       
	SpawnDefaultWeapon();

	UpdateHUDHealth();
	UpdateHUDShield();
	UpdateHUDAmmo();
	
	if (HasAuthority())
	{
		OnTakeAnyDamage.AddDynamic(this, &ABlasterCharacter::ReceiveDamage);
	}

	if (AttachedGrenade)
	{
		AttachedGrenade->SetVisibility(false);
	}
}

void ABlasterCharacter::SpawnDefaultWeapon()
{
	ABlasterGameMode* BlasterGameMode = Cast<ABlasterGameMode>(UGameplayStatics::GetGameMode(this));
	UWorld* World = GetWorld();
	if (BlasterGameMode && World && !bElimmed && DefaultWeapon)
	{
		AWeapon* StartingWeapon = World->SpawnActor<AWeapon>(DefaultWeapon);
		StartingWeapon->bDestroyWeapon = true;
		if (Combat)
		{
			Combat->EquipWeapon(StartingWeapon);
		}
	}
}

void ABlasterCharacter::Elim(bool bPlayerLeftGame)
{
	DropOrDestroyWeapons();

	// 클라이언트들에게 알림
	MulticastElim(bPlayerLeftGame);
}

void ABlasterCharacter::DropOrDestroyWeapons()
{
	if (Combat)
	{
		if (Combat->EquippedWeapon)
		{
			DropOrDestroyWeapon(Combat->EquippedWeapon);
		}
		if (Combat->SecondaryWeapon)
		{
			DropOrDestroyWeapon(Combat->SecondaryWeapon);
		}
	}
}

void ABlasterCharacter::DropOrDestroyWeapon(AWeapon* Weapon)
{
	if (Weapon == nullptr) return;
	
	// 무기 드랍
	if (Weapon->bDestroyWeapon)
	{
		Weapon->Destroy();
	}
	else
	{
		Weapon->Dropped();
	}
}

void ABlasterCharacter::MulticastElim_Implementation(bool bPlayerLeftGame)
{
	// 클라이언트가 세션을 나간 경우 true가 됨
	bLeftGame = bPlayerLeftGame;
	
	if (BlasterPlayerController)
	{
		BlasterPlayerController->SetHUDWeaponAmmo(0);
	}
	
	bElimmed = true;
	if (OverheadWidget)
	{
		UOverheadWidget* Widget = Cast<UOverheadWidget>(OverheadWidget->GetUserWidgetObject());
		if (Widget)
		{
			Widget->SetPlayerName(FString(""));
		}
	}
	PlayElimMontage();

	// Dissolve effect 시작
	if (DissolveMaterialInstance)
	{
		DynamicDissolveMaterialInstance = UMaterialInstanceDynamic::Create(DissolveMaterialInstance, this);
		GetMesh()->SetMaterial(0, DynamicDissolveMaterialInstance);
		DynamicDissolveMaterialInstance->SetScalarParameterValue(TEXT("Dissolve"), 0.55f);
		DynamicDissolveMaterialInstance->SetScalarParameterValue(TEXT("Glow"), 200.f);
	}
	StartDissolve();

	// 캐릭터 움직임 중단, 마우스 움직임으로 인한 캐릭터 회전도 중단
	GetCharacterMovement()->DisableMovement();
	GetCharacterMovement()->StopMovementImmediately();
	if (BlasterPlayerController)
	{
		// 마우스 인풋 중단
		DisableInput(BlasterPlayerController);
		BlasterPlayerController->bDisableGameplay = true;
	}
	
	// 0.2초 뒤에 콜리전 끄기
	// 바로 끄면 다른 클라이언트측 Projectile들이 Hit 이벤트를 검출하지 못 하는 경우가 있음
	FTimerHandle CollisionTimerHandle;
	GetWorldTimerManager().SetTimer(
	CollisionTimerHandle,
	[this]()
	{ 
	GetCapsuleComponent()->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	GetMesh()->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	},
	0.2f,
	false
	);

	// Elim bot 스폰
	if (ElimBotEffect)
	{
		FVector ElimBotSpawnPoint(GetActorLocation().X, GetActorLocation().Y, GetActorLocation().Z + 200.f);
		ElimBotComponent = UGameplayStatics::SpawnEmitterAtLocation(
			GetWorld(),
			ElimBotEffect,
			ElimBotSpawnPoint,
			GetActorRotation()
			);
	}
	if (ElimBotSound)
	{
		UGameplayStatics::SpawnSoundAtLocation(
			this,
			ElimBotSound,
			GetActorLocation()
			);
	}
	bool bHideSniperScope = IsLocallyControlled()
		&& Combat
		&& Combat->bAiming
		&& Combat->EquippedWeapon
		&& Combat->EquippedWeapon->GetWeaponType() == EWeaponType::EWT_SniperRifle;
	
	if (bHideSniperScope)
	{
		ShowSniperScopeWidget(false);
	}

	if (CrownComponent)
	{
		CrownComponent->DestroyComponent();
	}
	
	GetWorldTimerManager().SetTimer(
		ElimTimer,
		this,
		&ThisClass::ElimTimerFinished,
		ElimDelay
		);
}

void ABlasterCharacter::ElimTimerFinished()
{
	// ElimDelay만큼의 시간 이후 리스폰
	ABlasterGameMode* BlasterGameMode = GetWorld()->GetAuthGameMode<ABlasterGameMode>();
	if (BlasterGameMode && !bLeftGame)
	{
		BlasterGameMode->RequestRespawn(this, Controller);
	}
	// 클라이언트가 세션을 나간 경우 들어오는 분기
	if (bLeftGame && IsLocallyControlled() && OnLeftGame.IsBound())
	{
		OnLeftGame.Broadcast();
	}
}

void ABlasterCharacter::ServerLeaveGame_Implementation()
{
	ABlasterGameMode* BlasterGameMode = GetWorld()->GetAuthGameMode<ABlasterGameMode>();
	BlasterPlayerState = BlasterPlayerState == nullptr ? GetPlayerState<ABlasterPlayerState>() : BlasterPlayerState;
	if (BlasterGameMode && BlasterPlayerState)
	{
		BlasterGameMode->PlayerLeftGame(BlasterPlayerState);
	}
}

void ABlasterCharacter::Destroyed()
{
	Super::Destroyed();

	if (ElimBotComponent)
	{
		ElimBotComponent->DestroyComponent();
	}

	// 캐릭터가 Destroy될 때 게임이 InProgress 상태가 아니면 무기를 함께 Destroy함
	ABlasterGameMode* BlasterGameMode = Cast<ABlasterGameMode>(UGameplayStatics::GetGameMode(this));
	bool bMatchNotInProgress = BlasterGameMode && BlasterGameMode->GetMatchState() != MatchState::InProgress;
	
	if (Combat && Combat->EquippedWeapon && bMatchNotInProgress)
	{
		Combat->EquippedWeapon->Destroy();
	}
}

void ABlasterCharacter::SetOverlappingWeapon(AWeapon* Weapon)
{
	if (IsLocallyControlled())
	{
		// 추가되는 경우
		if (!OverlappingWeapons.Contains(Weapon))
		{
			if (!OverlappingWeapons.IsEmpty() && OverlappingWeapons.Top())
			{
				// 비어있지 않으면 위젯이 활성화된 Weapon 비활성화
				OverlappingWeapons.Top()->ShowPickupWidget(false);
			}
			// Overlap된 Weapon 추가 후 위젯 활성화
			OverlappingWeapons.Emplace(Weapon);
			Weapon->ShowPickupWidget(true);
		}
		// 제외되는 경우
		else
		{
			if (OverlappingWeapons.Num() == 1)
			{
				// 1개인 상태에서 제외되는 경우
				Weapon->ShowPickupWidget(false);
				OverlappingWeapons.Empty();
			}
			else if (OverlappingWeapons.Top() == Weapon)
			{
				// 2개 이상인 상태에서 제외되는 Weapon이 현재 위젯이 활성화된 Weapon인 경우
				Weapon->ShowPickupWidget(false);
				OverlappingWeapons.RemoveAt(OverlappingWeapons.Num() - 1);
				if (!OverlappingWeapons.IsEmpty() && OverlappingWeapons.Top())
				{
					OverlappingWeapons.Top()->ShowPickupWidget(true);
				}
			}
			else
			{
				// 활성화된 Weapon이 아닌 경우 단순 제거
				OverlappingWeapons.Remove(Weapon);
			}
		}
	}
	else
	{
		// 추가되는 경우
		if (!OverlappingWeapons.Contains(Weapon))
		{
			OverlappingWeapons.Emplace(Weapon);
		}
		// 제외되는 경우
		else
		{
			OverlappingWeapons.Remove(Weapon);
		}
	}
}

void ABlasterCharacter::OnRep_OverlappingWeapons(TArray<AWeapon*> LastWeapons)
{
	if (!LastWeapons.IsEmpty() && LastWeapons.Top())
	{
		LastWeapons.Top()->ShowPickupWidget(false);
	}
	
	if (!OverlappingWeapons.IsEmpty() && OverlappingWeapons.Top())
	{
		OverlappingWeapons.Top()->ShowPickupWidget(true);
	}
}

void ABlasterCharacter::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
	
	RotateInPlace(DeltaTime);
	HideCameraIfCharacterClose();
	
	PollInit();
}

void ABlasterCharacter::RotateInPlace(float DeltaTime)
{
	// 서버에 의해 원격 조작되는 캐릭터는 SimProxiesTurn을 사용
 	if (GetLocalRole() > ROLE_SimulatedProxy && IsLocallyControlled())
 	{
 		AimOffset(DeltaTime);		
 	}
 	else
 	{
 		TimeSinceLastMovementReplication += DeltaTime;		
 		if (TimeSinceLastMovementReplication > 0.25f)
 		{
 			OnRep_ReplicatedMovement();
 		}
 		CalculateAO_Pitch();
 	}
}

void ABlasterCharacter::OnRep_ReplicatedMovement()
{
	Super::OnRep_ReplicatedMovement();

	SimProxiesTurn();
	
	TimeSinceLastMovementReplication = 0.f;
}

float ABlasterCharacter::CalculateSpeed()
{
	FVector Velocity = GetVelocity();
	Velocity.Z = 0.f;
	return Velocity.Size();
}

void ABlasterCharacter::AimOffset(float DeltaTime)
{
	if (Combat && Combat->EquippedWeapon == nullptr) return;
	
	float Speed = CalculateSpeed();
	bool bIsInAir = GetCharacterMovement()->IsFalling();

	if (Speed == 0.f && !bIsInAir) // 가만히 서있고 점프 안 할 때
	{
		bRotateRootBone = true;
		FRotator CurrentAimRotation = FRotator(0.f, GetBaseAimRotation().Yaw, 0.f);
		FRotator DeltaAimRotation = UKismetMathLibrary::NormalizedDeltaRotator(CurrentAimRotation, StartingAimRotation);
		AO_Yaw = DeltaAimRotation.Yaw;
		if (TurningInPlace == ETurningInPlace::ETIP_NotTurning)
		{
			InterpAO_Yaw = AO_Yaw;
		}
		bUseControllerRotationYaw = true;
		TurnInPlace(DeltaTime);
	}
	if (Speed > 0.f || bIsInAir) // 움직이거나 점프 중일 때
	{
		bRotateRootBone = false;
		StartingAimRotation = FRotator(0.f, GetBaseAimRotation().Yaw, 0.f);
		AO_Yaw = 0.f;
		bUseControllerRotationYaw = true;
		TurningInPlace = ETurningInPlace::ETIP_NotTurning;
	}

	CalculateAO_Pitch();
}

void ABlasterCharacter::CalculateAO_Pitch()
{
	AO_Pitch = GetBaseAimRotation().Pitch;
	if (AO_Pitch > 90.f && !IsLocallyControlled())
	{
		// pitch의 270 ~ 360을 -90 ~ 0 으로 바꾸는 과정
		FVector2D InRange(270.f, 360.f);
		FVector2D OutRange(-90.f, 0.f);
		AO_Pitch = FMath::GetMappedRangeValueClamped(InRange, OutRange, AO_Pitch);
	}
}

void ABlasterCharacter::SimProxiesTurn()
{
	if (Combat == nullptr || Combat->EquippedWeapon == nullptr) return;

	bRotateRootBone = false;

	float Speed = CalculateSpeed();
	if (Speed > 0.f)
	{
		TurningInPlace = ETurningInPlace::ETIP_NotTurning;
		return;
	}

	ProxyRotationLastFrame = ProxyRotation;
	ProxyRotation = GetActorRotation();
	ProxyYaw = UKismetMathLibrary::NormalizedDeltaRotator(ProxyRotation, ProxyRotationLastFrame).Yaw;

	if (FMath::Abs(ProxyYaw) > TurnThreshold)
	{
		if (ProxyYaw > TurnThreshold)
		{
			TurningInPlace = ETurningInPlace::ETIP_Right;
		}
		else if (ProxyYaw < -TurnThreshold)
		{
			TurningInPlace = ETurningInPlace::ETIP_Left;
		}
		else
		{
			TurningInPlace = ETurningInPlace::ETIP_NotTurning;
		}
		return;
	}
	TurningInPlace = ETurningInPlace::ETIP_NotTurning;
}

void ABlasterCharacter::TurnInPlace(float DeltaTime)
{
	if (AO_Yaw > 90.f)
	{
		TurningInPlace = ETurningInPlace::ETIP_Right;
	}
	else if (AO_Yaw < -90.f)
	{
		TurningInPlace = ETurningInPlace::ETIP_Left;
	}
	if (TurningInPlace != ETurningInPlace::ETIP_NotTurning)
	{
		InterpAO_Yaw = FMath::FInterpTo(InterpAO_Yaw, 0.f, DeltaTime, 4.f);
		AO_Yaw = InterpAO_Yaw;
		if (FMath::Abs(AO_Yaw) < 15.f)
		{
			TurningInPlace = ETurningInPlace::ETIP_NotTurning;
			StartingAimRotation = FRotator(0.f, GetBaseAimRotation().Yaw, 0.f);
		}
	}
}

void ABlasterCharacter::ReceiveDamage(AActor* DamagedActor,
                                      float Damage,
                                      const UDamageType* DamageType,
                                      AController* InstigatorController,
                                      AActor* DamageCauser)
{
	if (bElimmed) return;
	
	float DamageToHealth = Damage;
	if (Shield > 0.f)
	{
		if (Shield >= Damage)
		{
			Shield = FMath::Clamp(Shield - Damage, 0.f, MaxShield);
			DamageToHealth = 0.f;
		}
		else
		{
			DamageToHealth = FMath::Clamp(Damage - Shield, 0.f, Damage);
			Shield = 0.f;
		}
	}
	
	// 서버에서 실행되는 함수 (서버가 권한을 가진 액터에서 ApplyDamage를 호출하는 것도 있지만, 애초에 서버에서만 바인딩했음)
	// Health는 클라이언트에 복제되어 OnRep_Health를 호출
	Health = FMath::Clamp(Health - DamageToHealth, 0.f, MaxHealth);

	// DamageType에 의한 추가 효과
	// const_cast를 사용한다는 것 자체가 구조에 문제가 있는 것으로 보임
	// 따라서 현재는 전혀 사용하지 않고 있음
	if (UDamageType* MutableDamageType = const_cast<UDamageType*>(DamageType))
	{
		if (UBaseDamageType* CastedDamageType = Cast<UBaseDamageType>(MutableDamageType))
		{
			CastedDamageType->ApplyDamageTypeEffect(DamagedActor, InstigatorController);
		}
	}

	// 서버인 경우 HUD 업데이트
	if (IsLocallyControlled())
	{
		UpdateHUDHealth();
		UpdateHUDShield();
	}

	// HP가 0에 도달할 경우 GameMode에게 사망했다고 알림
	if (Health == 0.f)
	{
		ABlasterGameMode* BlasterGameMode = GetWorld()->GetAuthGameMode<ABlasterGameMode>();
		if (BlasterGameMode)
		{
			BlasterPlayerController = BlasterPlayerController == nullptr ? Cast<ABlasterPlayerController>(Controller) : BlasterPlayerController;
			ABlasterPlayerController* AttackerController = Cast<ABlasterPlayerController>(InstigatorController);
			BlasterGameMode->PlayerEliminated(this, BlasterPlayerController, AttackerController);
			return;
		}
	}
	
	FVector_NetQuantize CausedLocation = DamageCauser->GetActorLocation();
	MulticastPlayHitReactMontage(CausedLocation);
}

void ABlasterCharacter::MulticastPlayHitReactMontage_Implementation(FVector_NetQuantize CausedLocation)
{
	if (!IsWeaponEquipped()) return;

	FVector HitDirection = CausedLocation - GetActorLocation();
	float HitRotationYaw = FRotator::ClampAxis(GetActorRotation().Yaw - HitDirection.Rotation().Yaw);

	FName SectionName("Null");
	if (315.f < HitRotationYaw || HitRotationYaw <= 45.f)
	{
		SectionName = FName("FromFront");
	}
	else if (45.f < HitRotationYaw && HitRotationYaw <= 135.f)
	{
		SectionName = FName("FromRight");
	}
	else if (135.f < HitRotationYaw && HitRotationYaw <= 225.f)
	{
		SectionName = FName("FromBack");
	}
	else if (225.f < HitRotationYaw && HitRotationYaw <= 315.f)
	{
		SectionName = FName("FromLeft");
	}

	if (SectionName != FName("Null"))
	{
		UAnimInstance* AnimInstance = GetMesh()->GetAnimInstance();
		if (AnimInstance && HitReactMontage)
		{
			AnimInstance->Montage_Play(HitReactMontage);
			AnimInstance->Montage_JumpToSection(SectionName);
		}
	}
}

void ABlasterCharacter::OnRep_Health(float LastHealth)
{
	UpdateHUDHealth();
}

void ABlasterCharacter::OnRep_Shield(float LastShield)
{
	UpdateHUDShield();
}

void ABlasterCharacter::UpdateHUDHealth()
{
	BlasterPlayerController = BlasterPlayerController == nullptr ? Cast<ABlasterPlayerController>(Controller) : BlasterPlayerController;
	if (BlasterPlayerController)
	{
		BlasterPlayerController->SetHUDHealth(Health, MaxHealth);
	}
}

void ABlasterCharacter::UpdateHUDShield()
{
	BlasterPlayerController = BlasterPlayerController == nullptr ? Cast<ABlasterPlayerController>(Controller) : BlasterPlayerController;
	if (BlasterPlayerController)
	{
		BlasterPlayerController->SetHUDShield(Shield, MaxShield);
	}
}

void ABlasterCharacter::UpdateHUDAmmo()
{
	BlasterPlayerController = BlasterPlayerController == nullptr ? Cast<ABlasterPlayerController>(Controller) : BlasterPlayerController;
	if (BlasterPlayerController && Combat)
	{
		BlasterPlayerController->SetHUDCarriedAmmo(Combat->CarriedAmmo);
		if (Combat->EquippedWeapon)
		{
			BlasterPlayerController->SetHUDWeaponAmmo(Combat->EquippedWeapon->GetAmmo());
		}
		else
		{
			BlasterPlayerController->SetHUDWeaponAmmo(0);
		}
	}
}

void ABlasterCharacter::PollInit()
{
	if (BlasterPlayerState == nullptr)
	{
		BlasterPlayerState = GetPlayerState<ABlasterPlayerState>();
		if (BlasterPlayerState)
		{
			BlasterPlayerState->AddToScore(0.f);
			BlasterPlayerState->AddToDefeats(0);

			ABlasterGameState* BlasterGameState = Cast<ABlasterGameState>(UGameplayStatics::GetGameState(this));

			if (BlasterGameState && BlasterGameState->TopScoringPlayers.Contains(BlasterPlayerState))
			{
				MulticastGainedTheLead();
			}
		}
	}
}

void ABlasterCharacter::PlayElimMontage()
{
	UAnimInstance* AnimInstance = GetMesh()->GetAnimInstance();
	if (AnimInstance && ElimMontage)
	{
		AnimInstance->Montage_Play(ElimMontage);
		const int8 DeathAnimNumber = FMath::RandRange(1, 5);
		const FName SectionName(*FString::Printf(TEXT("Death_%d"), DeathAnimNumber));
		AnimInstance->Montage_JumpToSection(SectionName);
	}
}

void ABlasterCharacter::PlayThrowGrenadeMontage()
{
	UAnimInstance* AnimInstance = GetMesh()->GetAnimInstance();
	if (AnimInstance && ThrowGrenadeMontage)
	{
		AnimInstance->Montage_Play(ThrowGrenadeMontage);
	}
}

void ABlasterCharacter::PlaySwapMontage()
{
	UAnimInstance* AnimInstance = GetMesh()->GetAnimInstance();
	if (AnimInstance && SwapMontage)
	{
		AnimInstance->Montage_Play(SwapMontage);
	}
}

void ABlasterCharacter::StartDissolve()
{
	// 시간에 따라 DissolveCurve대로 변화하는 값을 매개변수로 DissolveTrack에 바인딩된 함수를 프레임마다 호출
	DissolveTrack.BindDynamic(this, &ThisClass::UpdateDissolveMaterial);
	if (DissolveCurve && DissolveTimeline)
	{
		DissolveTimeline->AddInterpFloat(DissolveCurve, DissolveTrack);
		DissolveTimeline->Play();
	}
}

void ABlasterCharacter::UpdateDissolveMaterial(float DissolveValue)
{
	if (DynamicDissolveMaterialInstance)
	{
		DynamicDissolveMaterialInstance->SetScalarParameterValue("Dissolve", DissolveValue);
	}
}

void ABlasterCharacter::HideCameraIfCharacterClose()
{
	if (!IsLocallyControlled()) return;

	if ((FollowCamera->GetComponentLocation() - GetActorLocation()).Size() < CameraThreshold)
	{
		GetMesh()->SetVisibility(false);
		if (Combat && Combat->EquippedWeapon && Combat->EquippedWeapon->GetWeaponMesh())
		{
			Combat->EquippedWeapon->GetWeaponMesh()->bOwnerNoSee = true;
		}
	}
	else
	{
		GetMesh()->SetVisibility(true);
		if (Combat && Combat->EquippedWeapon && Combat->EquippedWeapon->GetWeaponMesh())
		{
			Combat->EquippedWeapon->GetWeaponMesh()->bOwnerNoSee = false;
		}
	}
}

void ABlasterCharacter::EquipButtonPressed()
{
	if (Combat && Combat->CombatState == ECombatState::ECS_Unoccupied)
	{
		// 클라이언트가 Equip버튼을 누른 경우 서버에게 착용을 요청
		ServerEquipButtonPressed();
	}
}

void ABlasterCharacter::ServerEquipButtonPressed_Implementation()
{
	if (Combat && !OverlappingWeapons.IsEmpty())
	{
		if (OverlappingWeapons.Top())
		{
			Combat->EquipWeapon(OverlappingWeapons.Top());
		}
	}
}

void ABlasterCharacter::SwapButtonPressed()
{
	if (Combat && Combat->EquippedWeapon && Combat->CombatState == ECombatState::ECS_Unoccupied)
	{
		ServerSwapButtonPressed();
		bool bSwap = Combat->ShouldSwapWeapons()
			&& !HasAuthority()
			&& !IsLocallyReloading();
		if (bSwap)
		{
			PlaySwapMontage();
			Combat->CombatState = ECombatState::ECS_SwappingWeapons;
			bFinishedSwapping = false;
		}
	}
}

void ABlasterCharacter::ServerSwapButtonPressed_Implementation()
{
	if (Combat && Combat->ShouldSwapWeapons())
	{
		Combat->SwapWeapons();
	}
}

void ABlasterCharacter::CrouchButtonPressed()
{
	if (bIsCrouched)
	{
		UnCrouch();
	}
	else
	{
		Crouch();	
	}
}

void ABlasterCharacter::AimButtonPressed()
{
	if (Combat)
	{
		Combat->SetAiming(true);
	}
}

void ABlasterCharacter::AimButtonReleased()
{
	if (Combat)
	{
		Combat->SetAiming(false);
	}
}

void ABlasterCharacter::Jump()
{
	if (bIsCrouched)
	{
		UnCrouch();
	}
	else
	{
		Super::Jump();
	}
}

void ABlasterCharacter::FireButtonPressed()
{
	if (Combat)
	{
		Combat->FireButtonPressed(true);
	}
}

void ABlasterCharacter::FireButtonReleased()
{
	if (Combat)
	{
		Combat->FireButtonPressed(false);
	}
}

void ABlasterCharacter::ReloadButtonPressed()
{
	if (Combat)
	{
		Combat->Reload();
	}
}

void ABlasterCharacter::GrenadeButtonPressed()
{
	if (Combat)
	{
		Combat->ThrowGrenade();
	}
}

void ABlasterCharacter::PlayFireMontage(bool bAiming)
{
	if (!IsWeaponEquipped()) return;

	UAnimInstance* AnimInstance = GetMesh()->GetAnimInstance();
	if (AnimInstance && FireWeaponMontage)
	{
		AnimInstance->Montage_Play(FireWeaponMontage);
		FName SectionName;
		SectionName = bAiming ? FName("RifleAim") : FName("RifleHip");
		AnimInstance->Montage_JumpToSection(SectionName);
	}
}

void ABlasterCharacter::PlayReloadMontage()
{
	if (!IsWeaponEquipped()) return;

	UAnimInstance* AnimInstance = GetMesh()->GetAnimInstance();
	if (AnimInstance && ReloadMontage)
	{
		AnimInstance->Montage_Play(ReloadMontage);
		const FWeaponTypes& WeaponTypes = FWeaponTypes::GetWeaponTypeInstance();
		const FName SectionName = WeaponTypes.WeaponName.FindRef(Combat->EquippedWeapon->GetWeaponType());
		
		AnimInstance->Montage_JumpToSection(SectionName);
	}
}

bool ABlasterCharacter::IsWeaponEquipped()
{
	return (Combat && Combat->EquippedWeapon);
}

bool ABlasterCharacter::IsAiming()
{
	return (Combat && Combat->bAiming);
}

AWeapon* ABlasterCharacter::GetEquippedWeapon()
{
	if (Combat == nullptr) return nullptr;
	
	return Combat->EquippedWeapon;
}

FVector ABlasterCharacter::GetHitTarget() const
{
	if (Combat == nullptr) return FVector();

	return Combat->HitTarget;
}

ECombatState ABlasterCharacter::GetCombatState() const
{
	if (Combat == nullptr) return ECombatState::ECS_MAX;
	
	return Combat->CombatState;
}

bool ABlasterCharacter::IsLocallyReloading()
{
	if (Combat == nullptr) return false;
	
	return Combat->bLocallyReloading;
}
