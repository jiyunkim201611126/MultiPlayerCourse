#include "CombatComponent.h"

#include "Blaster/Character/BlasterCharacter.h"
#include "Blaster/Weapon/Weapon.h"
#include "Engine/SkeletalMeshSocket.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Net/UnrealNetwork.h"
#include "Kismet/GameplayStatics.h"
#include "DrawDebugHelpers.h"
#include "Blaster/PlayerController/BlasterPlayerController.h"
#include "Camera/CameraComponent.h"
#include "TimerManager.h"
#include "Sound/SoundCue.h"
#include "Blaster/Weapon/Projectiles/Projectile.h"

UCombatComponent::UCombatComponent()
{
	PrimaryComponentTick.bCanEverTick = true;

	BaseWalkSpeed = 600.f;
	AimWalkSpeed = 350.f;
}

void UCombatComponent::GetLifetimeReplicatedProps(TArray<class FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(UCombatComponent, EquippedWeapon);
	DOREPLIFETIME_CONDITION(UCombatComponent, bAiming, COND_SkipOwner);
	DOREPLIFETIME_CONDITION(UCombatComponent, CarriedAmmo, COND_OwnerOnly);
	DOREPLIFETIME(UCombatComponent, CombatState);
	DOREPLIFETIME(UCombatComponent, Grenades);
}

void UCombatComponent::BeginPlay()
{
	Super::BeginPlay();
	UpdateHUDGrenades();

	if (Character)
	{
		Character->GetCharacterMovement()->MaxWalkSpeed = BaseWalkSpeed;

		// 기본 FOV와 현재 FOV 설정
		if (Character->GetFollowCamera())
		{
			DefaultFOV = Character->GetFollowCamera()->FieldOfView;
			CurrentFOV = DefaultFOV;
		}
		if (Character->HasAuthority())
		{
			InitializeCarriedAmmo();
		}
	}
}

void UCombatComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);
	
	// 자신의 캐릭터에만 조작할 것들
	if (Character && Character->IsLocallyControlled())
	{
		// 총구가 제대로 된 방향을 가리키도록 설정해줌
		FHitResult HitResult;
		HitTarget = TraceUnderCrosshairs(HitResult);

		// 크로스헤어 상태 조작
		SetHUDCrosshairs(DeltaTime);

		// 조준 상태에 따른 카메라 FOV 조작
		InterpFOV(DeltaTime);
	}
}

void UCombatComponent::FireButtonPressed(bool bPressed)
{
	if (EquippedWeapon == nullptr || Character == nullptr)
	{
		return;
	}
	
	// 일단 바로 변경, 아직 다른 클라이언트는 반영되지 않음
	bFireButtonPressed = bPressed;

	if (bFireButtonPressed)
	{
		Fire();
	}
}

void UCombatComponent::Fire()
{
	if (CanFire())
	{
		bCanFire = false;
		
		// 사격 시작
		FHitResult HitResult;
		LocalFire(TraceUnderCrosshairs(HitResult));

		if (EquippedWeapon && EquippedWeapon->bUseScatter)
		{
			// 비조준 사격 시 탄퍼짐 수치 계산
			CrosshairShootingFactor += EquippedWeapon->GetHipFireAccurateSubtract();
			CrosshairShootingFactor = FMath::Clamp(CrosshairShootingFactor, 0.f, EquippedWeapon->GetHipFireAccurateMaxSubtract());
		}
	
		// 일정 시간 후 다시 발사되도록 타이머 시작
		StartFireTimer();
	}
}

void UCombatComponent::StartFireTimer()
{
	if (EquippedWeapon == nullptr || Character == nullptr)
	{
		return;
	}
	
	Character->GetWorldTimerManager().SetTimer(
		FireTimer,							// 해당 타이머를 추적하는 변수
		this,
		&UCombatComponent::FireTimerFinished,	// 시간 경과 후 호출될 함수
		EquippedWeapon->FireDelay				// 해당 시간
		);
}

void UCombatComponent::FireTimerFinished()
{
	if (EquippedWeapon == nullptr)
	{
		return;
	}
	
	// FireDelay만큼의 시간 이후 사격 가능 상태로 변경
	// 이로 인해 단발 무기는 꾹 누르고 있어도 발사가 안 됨
	bCanFire = true;
	
	// FireDelay만큼의 시간 후, 아직 마우스를 누르고 있으면서 연사 무기인 경우 다시 Fire가 실행
	if (bFireButtonPressed && EquippedWeapon->bAutomatic)
	{
		Fire();
	}

	ReloadEmptyWeapon();
}

void UCombatComponent::LocalFire(const FVector_NetQuantize& TraceHitTarget)
{
	// 착용 중인 무기가 없는 경우 바로 리턴
	if (EquippedWeapon == nullptr)
	{
		return;
	}
	
	if (Character && Character->IsLocallyControlled() && CombatState == ECombatState::ECS_Unoccupied)
	{
		// 실제 사격에 대한 권한은 서버에게 있어야 하므로 애니메이션만 재생
		Character->PlayFireMontage(bAiming);
		EquippedWeapon->PlayFireMontage();
		
		// 서버에 사격 요청
		ServerFire(TraceHitTarget);
		return;
	}

	// 샷건인 경우 장전 중에도 예외로 사격 가능
	if (CombatState == ECombatState::ECS_Reloading && EquippedWeapon->GetWeaponType() == EWeaponType::EWT_Shotgun)
	{
		Character->PlayFireMontage(bAiming);
		EquippedWeapon->PlayFireMontage();
		
		// 서버에 사격 요청
		ServerFire(TraceHitTarget);
	}
}

void UCombatComponent::ServerFire_Implementation(const FVector_NetQuantize& TraceHitTarget)
{
	// 해당 함수는 ServerFire는 클라이언트가 서버에 요청하는 함수
	MulticastFire(TraceHitTarget);
}

void UCombatComponent::MulticastFire_Implementation(const FVector_NetQuantize& TraceHitTarget)
{
	// 서버가 클라이언트들에게 함수를 원격 호출해주는 함수
	
	if (EquippedWeapon == nullptr || Character == nullptr)
	{
		return;
	}

	// 로컬에선 이미 애니메이션을 재생했으므로, 내가 아닌 캐릭터만 애니메이션 재생
	if (!Character->IsLocallyControlled())
	{
		// 기본 상태거나, 장전 중이더라도 샷건인 경우 발사 가능
		if (CombatState == ECombatState::ECS_Unoccupied)
		{
			Character->PlayFireMontage(bAiming);
			EquippedWeapon->PlayFireMontage();
		}
		else if (CombatState == ECombatState::ECS_Reloading && EquippedWeapon->GetWeaponType() == EWeaponType::EWT_Shotgun)
		{
			Character->PlayFireMontage(bAiming);
			EquippedWeapon->PlayFireMontage();
			CombatState = ECombatState::ECS_Unoccupied;
		}
	}
	else
	{
		if (CombatState == ECombatState::ECS_Reloading && EquippedWeapon->GetWeaponType() == EWeaponType::EWT_Shotgun)
		{
			CombatState = ECombatState::ECS_Unoccupied;
		}
	}

	// 장착한 무기에 따라 다른 Fire 함수 호출
	// 서버의 Projectile 스폰, Replicates가 true인 액터이므로 모두에게 스폰
	// HitScan은 라인 트레이스 시작
	EquippedWeapon->Fire(TraceHitTarget);
}

FVector_NetQuantize UCombatComponent::TraceUnderCrosshairs(FHitResult& TraceHitResult)
{
	if (!HUD)
	{
		return FVector_NetQuantize::ZeroVector;
	}
	
	const FVector2D CrosshairLocation = HUD->GetCrosshairLocation();
	
	// 2D 화면 좌표를 3D 월드 좌표로 변환
	FVector CrosshairWorldPosition;		// 크로스헤어 시작 위치
	FVector CrosshairWorldDirection;	// 크로스헤어가 향하는 방향
	bool bScreenToWorld = UGameplayStatics::DeprojectScreenToWorld(
		UGameplayStatics::GetPlayerController(this, 0),
		CrosshairLocation,
		CrosshairWorldPosition,
		CrosshairWorldDirection
	);

	// 시작점과 끝점 계산
	FVector Start = CrosshairWorldPosition;

	if (Character)
	{
		float DistanceToCharacter = (Character->GetActorLocation() - Start).Size();
		Start += CrosshairWorldDirection * (DistanceToCharacter + 100.f);
	}
	
	const FVector End = Start + CrosshairWorldDirection * TRACE_LENGTH;
	
	// HitTarget 추적 시작
	if (bScreenToWorld)
	{
		// 두 벡터 중간에 Visibility 채널로 충돌 검출
		GetWorld()->LineTraceSingleByChannel(
			TraceHitResult,
			Start,
			End,
			ECollisionChannel::ECC_Visibility
			);

		// Hit한 액터가 있으며, 해당 액터가 InteractWithCrosshairsInterface를 상속받는 경우
		if (TraceHitResult.GetActor() && TraceHitResult.GetActor()->Implements<UInteractWithCrosshairsInterface>())
		{
			HUDPackage.CrosshairsColor = FLinearColor(1.f, 0.f, 0.f, 0.7f);
		}
		else
		{
			HUDPackage.CrosshairsColor = FLinearColor(1.f, 1.f, 1.f, 0.7f);
		}
	}
	
	if (TraceHitResult.bBlockingHit)
	{
		return TraceHitResult.ImpactPoint;
	}
	else
	{
		return End;
	}
}

void UCombatComponent::SetHUDCrosshairs(float DeltaTime)
{
	if (Character == nullptr || Character->Controller == nullptr)
	{
		return;
	}

	Controller = Controller == nullptr ? Cast<ABlasterPlayerController>(Character->Controller) : Controller;

	if (Controller)
	{
		HUD = HUD == nullptr ? Cast<ABlasterHUD>(Controller->GetHUD()) : HUD;
		if (HUD)
		{
			if (EquippedWeapon)
			{
				HUDPackage.CrosshairsCenter = EquippedWeapon->CrosshairsCenter;
				HUDPackage.CrosshairsLeft = EquippedWeapon->CrosshairsLeft;
				HUDPackage.CrosshairsRight = EquippedWeapon->CrosshairsRight;
				HUDPackage.CrosshairsTop = EquippedWeapon->CrosshairsTop;
				HUDPackage.CrosshairsBottom = EquippedWeapon->CrosshairsBottom;
				HUDPackage.CrosshairSpread = 0.f;
			}
			else
			{
				HUDPackage.CrosshairsCenter = nullptr;
				HUDPackage.CrosshairsLeft = nullptr;
				HUDPackage.CrosshairsRight = nullptr;
				HUDPackage.CrosshairsTop = nullptr;
				HUDPackage.CrosshairsBottom = nullptr;
				HUDPackage.CrosshairSpread = 0.f;
			}
			
			if (!EquippedWeapon || !EquippedWeapon->bUseScatter)
			{
				HUD->SetHUDPackage(HUDPackage);
				return;
			}
			
			// 이동속도에 대한 크로스헤어 스프레드 (현재 이동속도를 0 ~ 1 사이로 Clamp)
			FVector2D WalkSpeedRange(0.f, Character->GetCharacterMovement()->MaxWalkSpeed);
			FVector2D VelocityMultiplierRange(0.f, 1.f);
			FVector Velocity = Character->GetVelocity();
			CrosshairVelocityFactor = FMath::GetMappedRangeValueClamped(WalkSpeedRange, VelocityMultiplierRange, Velocity.Size2D());

			// 체공에 대한 크로스헤어 스프레드
			if (Character->GetCharacterMovement()->IsFalling())
			{
				CrosshairInAirFactor = FMath::FInterpTo(CrosshairInAirFactor, 2.25f, DeltaTime, 2.25f);
			}
			else
			{
				CrosshairInAirFactor = FMath::FInterpTo(CrosshairInAirFactor, 0.f, DeltaTime, 30.f);
			}

			// 조준에 대한 스프레드
			// 현재는 무기가 없으면 크로스헤어도 없으나 나중에 생길 수도 있으니 일단 없는 것도 구현
			const float TargetAimFactor = bAiming ? (EquippedWeapon ? EquippedWeapon->GetZoomAccurate() : 0.6f) : 0.f;
			CrosshairAimFactor = FMath::FInterpTo(CrosshairAimFactor, TargetAimFactor, DeltaTime, 30.f);

			// 사격에 대한 스프레드를 0으로 수렴, 수렴 속도는 조준 상태에 따라 다름.
			const float ShootingInterpSpeed = bAiming ? 20.f : 10.f;
			CrosshairShootingFactor = FMath::FInterpTo(CrosshairShootingFactor, 0.f, DeltaTime, ShootingInterpSpeed);

			// 최종 추가 탄퍼짐 계산
			float CrosshairSpreadResult =
				+ CrosshairVelocityFactor		// 이동 속도에 비례해 +
				+ CrosshairInAirFactor			// 체공 중 +
				+ CrosshairShootingFactor		// 사격 시 +
				- CrosshairAimFactor;			// 조준 시 -
			
			EquippedWeapon->SpreadFactor = CrosshairSpreadResult;
			EquippedWeapon->SpreadFactor = FMath::FInterpTo(EquippedWeapon->SpreadFactor, 0.f, DeltaTime, ShootingInterpSpeed);

			// 최종 크로스헤어 스프레드 수치 계산
			CrosshairSpreadResult += EquippedWeapon->DefaultSpreadFactor / 50.f;

			// 음수가 되어 크로스헤어들이 서로를 가로지르는 현상을 방지
			HUDPackage.CrosshairSpread = FMath::Clamp(CrosshairSpreadResult, 0.f, 100.f);
			
			HUD->SetHUDPackage(HUDPackage);
		}
	}
}

void UCombatComponent::InterpFOV(float DeltaTime)
{
	if (EquippedWeapon == nullptr)
	{
		return;
	}

	// 조준 상태에 따라 카메라의 FOV를 조작
	if (bAiming)
	{
		CurrentFOV =								// CurrentFOV를
			FMath::FInterpTo(						// 보간한다
			CurrentFOV,								// 현재 값에서부터
			EquippedWeapon->GetZoomedFOV(),			// EquippedWeapon의 ZoomedFOV로
			DeltaTime,								// DeltaTime에 따라
			EquippedWeapon->GetZoomInterpSpeed());	// ZoomInterpSpeed의 속도로
	}
	else
	{
		CurrentFOV =								// CurrentFOV를
			FMath::FInterpTo(						// 보간한다
			CurrentFOV,								// 현재 값에서부터
			DefaultFOV,								// DefaultFOV(카메라의 기본 FOV)로
			DeltaTime,								// DeltaTime에 따라
			EquippedWeapon->GetZoomInterpSpeed());	// ZoomInterpSpeed의 속도로
	}

	if (Character && Character->GetFollowCamera())
	{
		Character->GetFollowCamera()->SetFieldOfView(CurrentFOV);
	}
}

void UCombatComponent::SetAiming(bool bIsAiming)
{
	if (Character == nullptr || EquippedWeapon == nullptr)
	{
		return;
	}
	
	// 일단 바로 변경, 아직 다른 클라이언트는 반영되지 않음
	bAiming = bIsAiming;

	// 클라이언트인 경우 Aim을 서버에 요청
	ServerSetAiming(bIsAiming);
	
	if (Character)
	{
		Character->GetCharacterMovement()->MaxWalkSpeed = bIsAiming ? AimWalkSpeed : BaseWalkSpeed;
	}
	if (Character->IsLocallyControlled() && EquippedWeapon->GetWeaponType() == EWeaponType::EWT_SniperRifle)
	{
		Character->ShowSniperScopeWidget(bIsAiming);
	}
}

void UCombatComponent::ServerSetAiming_Implementation(bool bIsAiming)
{
	// 서버에서만 호출되는 함수이나, bAiming의 Replicated 속성과 COND_SkipOwner로 인해 자동으로 다른 클라이언트에도 반영
	bAiming = bIsAiming;
	
	if (Character)
	{
		Character->GetCharacterMovement()->MaxWalkSpeed = bIsAiming ? AimWalkSpeed : BaseWalkSpeed;
	}
}

void UCombatComponent::EquipWeapon(AWeapon* WeaponToEquip)
{
	if (Character == nullptr || WeaponToEquip == nullptr || CombatState != ECombatState::ECS_Unoccupied)
	{
		return;
	}

	DropEquippedWeapon();

	// 무기 장착, EquippedWeapon의 UPROPERTY로 인해 OnRep_EquippedWeapon이 자동 실행
	// 하지만 서버에선 자동으로 실행되지 않으므로 서버에서도 알 수 있게 아래에서 한 번 실행
	EquippedWeapon = WeaponToEquip;
	EquippedWeapon->SetWeaponState(EWeaponState::EWS_Equipped);

	AttachActorToRightHand(EquippedWeapon);

	EquippedWeapon->SetOwner(Character);
	EquippedWeapon->SetHUDAmmo();
	
	UpdateCarriedAmmo();
	PlayEquipWeaponSound();
	ReloadEmptyWeapon();
	
	Character->GetCharacterMovement()->bOrientRotationToMovement = false;
	Character->bUseControllerRotationYaw = true;
}

void UCombatComponent::DropEquippedWeapon()
{
	if (EquippedWeapon)
	{
		EquippedWeapon->Dropped();
	}
}

void UCombatComponent::AttachActorToRightHand(AActor* ActorToAttach)
{
	if (Character == nullptr || !Character->GetMesh() || ActorToAttach == nullptr)
	{
		return;
	}
	
	if (const USkeletalMeshSocket* HandSocket = Character->GetMesh()->GetSocketByName(FName("RightHandSocket")))
	{
		HandSocket->AttachActor(ActorToAttach, Character->GetMesh());
	}
}

void UCombatComponent::AttachActorToLeftHand(AActor* ActorToAttach)
{
	if (Character == nullptr || !Character->GetMesh() || ActorToAttach == nullptr || EquippedWeapon == nullptr)
	{
		return;
	}
	
	bool bUsePistolSocket =
		EquippedWeapon->GetWeaponType() == EWeaponType::EWT_Pistol
		|| EquippedWeapon->GetWeaponType() == EWeaponType::EWT_SubmachineGun;

	FName SocketName = bUsePistolSocket ? FName("PistolSocket") : FName("LeftHandSocket");
	if (const USkeletalMeshSocket* HandSocket = Character->GetMesh()->GetSocketByName(SocketName))
	{
		HandSocket->AttachActor(ActorToAttach, Character->GetMesh());
	}
}

void UCombatComponent::UpdateCarriedAmmo()
{
	if (EquippedWeapon == nullptr)
	{
		return;
	}
	
	if (CarriedAmmoMap.Contains(EquippedWeapon->GetWeaponType()))
	{
		CarriedAmmo = CarriedAmmoMap[EquippedWeapon->GetWeaponType()];
	}
	
	Controller = Controller == nullptr ? Cast<ABlasterPlayerController>(Character->Controller) : Controller;
	if (Controller)
	{
		Controller->SetHUDCarriedAmmo(CarriedAmmo);
	}
}

void UCombatComponent::PlayEquipWeaponSound()
{
	if (Character && EquippedWeapon && EquippedWeapon->EquipSound)
	{
		UGameplayStatics::PlaySoundAtLocation(
			this,
			EquippedWeapon->EquipSound,
			Character->GetActorLocation()
			);
	}
}

void UCombatComponent::ReloadEmptyWeapon()
{
	if (EquippedWeapon && EquippedWeapon->IsEmpty())
	{
		Reload();
	}
}

void UCombatComponent::Reload()
{
	if (EquippedWeapon
		&& EquippedWeapon->GetMagCapacity() > EquippedWeapon->GetAmmo()
		&& CarriedAmmo > 0
		&& CombatState == ECombatState::ECS_Unoccupied
		&& !EquippedWeapon->IsFull())
	{
		ServerReload();
	}
}

void UCombatComponent::ServerReload_Implementation()
{
	if (Character == nullptr || EquippedWeapon == nullptr)
	{
		return;
	}	

	// 상태 변경 후 애니메이션 재생 함수 호출
	CombatState = ECombatState::ECS_Reloading;
	HandleReload();
}

void UCombatComponent::HandleReload()
{
	Character->PlayReloadMontage();
}

void UCombatComponent::FinishReloading()
{
	if (Character == nullptr)
	{
		return;
	}
	if (Character->HasAuthority())
	{
		CombatState = ECombatState::ECS_Unoccupied;
		UpdateAmmoValues();
	}
	
	// 좌클릭 누르고 있는 상태면 사격
	if (bFireButtonPressed)
	{
		Fire();
	}
}

void UCombatComponent::UpdateAmmoValues()
{
	if (Character == nullptr || EquippedWeapon == nullptr)
	{
		return;
	}
	
	int32 ReloadAmount = AmountToReload();
	if (CarriedAmmoMap.Contains(EquippedWeapon->GetWeaponType()))
	{
		CarriedAmmoMap[EquippedWeapon->GetWeaponType()] -= ReloadAmount;
		CarriedAmmo = CarriedAmmoMap[EquippedWeapon->GetWeaponType()];
	}
	Controller = Controller == nullptr ? Cast<ABlasterPlayerController>(Character->Controller) : Controller;
	if (Controller)
	{
		Controller->SetHUDCarriedAmmo(CarriedAmmo);
	}
	EquippedWeapon->AddAmmo(ReloadAmount);
}

void UCombatComponent::ShotgunShellReload()
{
	if (Character && Character->HasAuthority())
	{
		UpdateShotgunAmmoValues();
	}
}

void UCombatComponent::UpdateShotgunAmmoValues()
{
	if (Character == nullptr || EquippedWeapon == nullptr)
	{
		return;
	}

	if (CarriedAmmoMap.Contains(EquippedWeapon->GetWeaponType()))
	{
		CarriedAmmoMap[EquippedWeapon->GetWeaponType()] -= 1;
		CarriedAmmo = CarriedAmmoMap[EquippedWeapon->GetWeaponType()];
	}
	Controller = Controller == nullptr ? Cast<ABlasterPlayerController>(Character->Controller) : Controller;
	if (Controller)
	{
		Controller->SetHUDCarriedAmmo(CarriedAmmo);
	}
	EquippedWeapon->AddAmmo(1);

	bool bNeedMoreReload = !EquippedWeapon->IsFull() && CarriedAmmo > 0;
	JumpToShotgunMoreReload(bNeedMoreReload);
}

void UCombatComponent::JumpToShotgunMoreReload(bool bNeedMoreReload)
{
	UAnimInstance* AnimInstance = Character->GetMesh()->GetAnimInstance();
	if (AnimInstance && Character->GetReloadMontage())
	{
		if (bNeedMoreReload)
		{
			AnimInstance->Montage_JumpToSection(FName("ShotgunStart"));
		}
		else
		{
			AnimInstance->Montage_JumpToSection(FName("ShotgunEnd"));
		}
	}
}

int32 UCombatComponent::AmountToReload()
{
	if (EquippedWeapon == nullptr)
	{
		return 0;
	}

	const int32 RoomInMag = EquippedWeapon->GetMagCapacity() - EquippedWeapon->GetAmmo();
	
	if (CarriedAmmoMap.Contains(EquippedWeapon->GetWeaponType()))
	{
		const int32 AmountCarried = CarriedAmmoMap[EquippedWeapon->GetWeaponType()];
		const int32 Least = FMath::Min(RoomInMag, AmountCarried);
		return FMath::Clamp(RoomInMag, 0, Least);
	}
	
	return int32();
}

void UCombatComponent::PickupAmmo(EWeaponType WeaponType, int32 AmmoAmount)
{
	if (CarriedAmmoMap.Contains(WeaponType))
	{
		CarriedAmmoMap[WeaponType] = FMath::Clamp(CarriedAmmoMap[WeaponType] + AmmoAmount, 0, MaxCarriedAmmo);

		UpdateCarriedAmmo();
	}
	if (EquippedWeapon && EquippedWeapon->IsEmpty() && EquippedWeapon->GetWeaponType() == WeaponType)
	{
		Reload();
	}
}

void UCombatComponent::ThrowGrenade()
{
	if (Grenades == 0 || CombatState != ECombatState::ECS_Unoccupied || EquippedWeapon == nullptr)
	{
		return;
	}
	
	CombatState = ECombatState::ECS_ThrowingGrenade;
	if (Character)
	{
		Character->PlayThrowGrenadeMontage();
		AttachActorToLeftHand(EquippedWeapon);
		ShowAttachedGrenade(true);
	}
	if (Character && !Character->HasAuthority())
	{
		ServerThrowGrenade();
	}
	if (Character && Character->HasAuthority())
	{
		Grenades = FMath::Clamp(Grenades - 1, 0, MaxGrenades);
		UpdateHUDGrenades();
	}
}

void UCombatComponent::ShowAttachedGrenade(bool bShowGrenade)
{
	if (Character && Character->GetAttachedGrenade())
	{
		Character->GetAttachedGrenade()->SetVisibility(bShowGrenade);
	}
}

void UCombatComponent::ServerThrowGrenade_Implementation()
{
	if (Grenades == 0)
	{
		return;
	}
	
	CombatState = ECombatState::ECS_ThrowingGrenade;
	if (Character)
	{
		Character->PlayThrowGrenadeMontage();
		AttachActorToLeftHand(EquippedWeapon);
		ShowAttachedGrenade(true);
	}
	Grenades = FMath::Clamp(Grenades - 1, 0, MaxGrenades);
	UpdateHUDGrenades();
}

void UCombatComponent::OnRep_Grenades()
{
	UpdateHUDGrenades();
}

void UCombatComponent::UpdateHUDGrenades()
{
	Controller = Controller == nullptr ? Cast<ABlasterPlayerController>(Character->Controller) : Controller;
	if (Controller)
	{
		Controller->SetHUDGrenades(Grenades);
	}
}

void UCombatComponent::LaunchGrenade()
{
	ShowAttachedGrenade(false);
	if (Character && Character->IsLocallyControlled())
	{
		ServerLaunchGrenade(HitTarget);
	}
}

void UCombatComponent::ServerLaunchGrenade_Implementation(const FVector_NetQuantize& Target)
{
	if (Character && GrenadeClass && Character->GetAttachedGrenade())
	{
		FVector StartingLocation = Character->GetAttachedGrenade()->GetComponentLocation();
		FVector ToTarget = Target - StartingLocation;
		StartingLocation += ToTarget / ToTarget.Size() * 50.f;
		FActorSpawnParameters SpawnParams;
		SpawnParams.Owner = Character;
		SpawnParams.Instigator = Character;
		if (UWorld* World = GetWorld())
		{
			World->SpawnActor<AProjectile>(
				GrenadeClass,
				StartingLocation,
				ToTarget.Rotation(),
				SpawnParams
				);
		}
	}
}

void UCombatComponent::ThrowGrenadeFinished()
{
	CombatState = ECombatState::ECS_Unoccupied;
	AttachActorToRightHand(EquippedWeapon);
}

void UCombatComponent::OnRep_CarriedAmmo()
{	
	Controller = Controller == nullptr ? Cast<ABlasterPlayerController>(Character->Controller) : Controller;
	if (Controller)
	{
		Controller->SetHUDCarriedAmmo(CarriedAmmo);
	}
	bool bJumpToShotgunEnd = CombatState == ECombatState::ECS_Reloading
		&& EquippedWeapon != nullptr
		&& EquippedWeapon->GetWeaponType() == EWeaponType::EWT_Shotgun
		&& (CarriedAmmo == 0 || EquippedWeapon->IsEmpty());
	JumpToShotgunMoreReload(!bJumpToShotgunEnd);
}

void UCombatComponent::OnRep_CombatState()
{
	switch (CombatState)
	{
	case ECombatState::ECS_Unoccupied:
		if (bFireButtonPressed)
		{
			Fire();
		}
		break;
	case ECombatState::ECS_Reloading:
		HandleReload();
		break;
	case ECombatState::ECS_ThrowingGrenade:
		if (Character && !Character->IsLocallyControlled())
		{
			Character->PlayThrowGrenadeMontage();
			AttachActorToLeftHand(EquippedWeapon);
			ShowAttachedGrenade(true);
		}
	}
}

void UCombatComponent::OnRep_EquippedWeapon()
{
	if (Character && EquippedWeapon)
	{
		EquippedWeapon->SetWeaponState(EWeaponState::EWS_Equipped);
		AttachActorToRightHand(EquippedWeapon);
		PlayEquipWeaponSound();
		
		Character->GetCharacterMovement()->bOrientRotationToMovement = false;
		Character->bUseControllerRotationYaw = true;
	}
}

bool UCombatComponent::CanFire()
{
	if (EquippedWeapon == nullptr)
	{
		return false;
	}

	if (!EquippedWeapon->IsEmpty() && bCanFire && CombatState == ECombatState::ECS_Reloading && EquippedWeapon->GetWeaponType() == EWeaponType::EWT_Shotgun)
	{
		// 샷건 장전 중엔 예외로 사격 가능
		return true;
	}

	return !EquippedWeapon->IsEmpty() && bCanFire && CombatState == ECombatState::ECS_Unoccupied;
}

void UCombatComponent::InitializeCarriedAmmo()
{
	CarriedAmmoMap.Emplace(EWeaponType::EWT_AssaultRifle, StartingARAmmo);
	CarriedAmmoMap.Emplace(EWeaponType::EWT_RocketLauncher, StartingRocketAmmo);
	CarriedAmmoMap.Emplace(EWeaponType::EWT_Pistol, StartingPistolAmmo);
	CarriedAmmoMap.Emplace(EWeaponType::EWT_SubmachineGun, StartingSMGAmmo);
	CarriedAmmoMap.Emplace(EWeaponType::EWT_Shotgun, StartingShotgunAmmo);
	CarriedAmmoMap.Emplace(EWeaponType::EWT_SniperRifle, StartingSniperAmmo);
	CarriedAmmoMap.Emplace(EWeaponType::EWT_GrenadeLauncher, StartingGrenadeLauncherAmmo);
}
