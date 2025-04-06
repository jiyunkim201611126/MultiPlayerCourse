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
#include "Blaster/Weapon/Shotgun.h"

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
	DOREPLIFETIME(UCombatComponent, SecondaryWeapon);
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
	if (EquippedWeapon == nullptr || Character == nullptr) return;
	
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
		EquippedWeapon->bCanFire = false;

		switch (EquippedWeapon->FireType)
		{
		case EFireType::EFT_Projectile:
			FireProjectileWeapon();
			break;
		case EFireType::EFT_HitScan:
			FireHitScanWeapon();
			break;
		case EFireType::EFT_Shotgun:
			FireShotgun();
			break;
		}

		if (EquippedWeapon->bUseScatter)
		{
			// 비조준 사격 시 탄퍼짐 수치 계산
			CrosshairShootingFactor += EquippedWeapon->GetHipFireAccurateSubtract();
			CrosshairShootingFactor = FMath::Clamp(CrosshairShootingFactor, 0.f, EquippedWeapon->GetHipFireAccurateMaxSubtract());
		}

		// 타이머가 바인드되지 않은 경우 다시 바인드
		if (!EquippedWeapon->OnFireTimerFinished.IsBound())
		{
			MulticastBindFireTimer(EquippedWeapon, true);
		}
		
		// 일정 시간 후 다시 발사되도록 타이머 시작
		EquippedWeapon->StartFireTimer();
	}
}

void UCombatComponent::FireProjectileWeapon()
{
	if (EquippedWeapon && Character)
	{
		HitTarget = EquippedWeapon->bUseScatter ? EquippedWeapon->TraceEndWithScatter(HitTarget) : HitTarget;
		if (!Character->HasAuthority())
		{
			LocalFire(HitTarget);
		}
		ServerFire(HitTarget);
	}
}

void UCombatComponent::FireHitScanWeapon()
{
	if (EquippedWeapon && Character)
	{
		HitTarget = EquippedWeapon->bUseScatter ? EquippedWeapon->TraceEndWithScatter(HitTarget) : HitTarget;
		if (!Character->HasAuthority())
		{
			LocalFire(HitTarget);
		}
		ServerFire(HitTarget);
	}
}

void UCombatComponent::FireShotgun()
{
	AShotgun* Shotgun = Cast<AShotgun>(EquippedWeapon);
	if (Shotgun && Character)
	{
		TArray<FVector_NetQuantize> HitTargets;
		Shotgun->ShotgunTraceEndWithScatter(HitTarget, HitTargets);
		if (!Character->HasAuthority())
		{
			LocalShotgunFire(HitTargets);
		}
		ServerShotgunFire(HitTargets);
	}
}

void UCombatComponent::LocalFire(const FVector_NetQuantize& TraceHitTarget)
{
	if (Character == nullptr || EquippedWeapon == nullptr) return;
	
	if (Character && CombatState == ECombatState::ECS_Unoccupied)
	{
		// 실제 사격에 대한 권한은 서버에게 있어야 하므로 애니메이션만 재생
		Character->PlayFireMontage(bAiming);
		EquippedWeapon->Fire(TraceHitTarget);
	}
}

void UCombatComponent::LocalShotgunFire(const TArray<FVector_NetQuantize>& TraceHitTargets)
{
	AShotgun* Shotgun = Cast<AShotgun>(EquippedWeapon);
	if (Shotgun == nullptr || Character == nullptr) return;

	if (CombatState == ECombatState::ECS_Reloading || CombatState == ECombatState::ECS_Unoccupied)
	{
		Character->PlayFireMontage(bAiming);
		Shotgun->FireShotgun(TraceHitTargets);
		CombatState = ECombatState::ECS_Unoccupied;
	}
}

void UCombatComponent::ServerFire_Implementation(const FVector_NetQuantize& TraceHitTarget)
{
	// 해당 함수는 ServerFire는 클라이언트가 서버에 요청하는 함수
	MulticastFire(TraceHitTarget);
}

void UCombatComponent::MulticastFire_Implementation(const FVector_NetQuantize& TraceHitTarget)
{
	// 자신의 캐릭터는 이미 Local에서 애니메이션 재생과 Fire를 마쳤으므로 바로 return
	if (Character && Character->IsLocallyControlled() && !Character->HasAuthority()) return;
	
	LocalFire(TraceHitTarget);
}

void UCombatComponent::ServerShotgunFire_Implementation(const TArray<FVector_NetQuantize>& TraceHitTargets)
{
	MulticastShotgunFire(TraceHitTargets);
}

void UCombatComponent::MulticastShotgunFire_Implementation(const TArray<FVector_NetQuantize>& TraceHitTargets)
{
	// 자신의 캐릭터는 이미 Local에서 애니메이션 재생과 Fire를 마쳤으므로 바로 return
	if (Character && Character->IsLocallyControlled() && !Character->HasAuthority()) return;
	
	LocalShotgunFire(TraceHitTargets);
}

FVector_NetQuantize UCombatComponent::TraceUnderCrosshairs(FHitResult& TraceHitResult)
{
	if (!HUD) return FVector_NetQuantize::ZeroVector;
	
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
	if (Character == nullptr || Character->Controller == nullptr) return;

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
			const float ShootingFactorRecoverySpeed = bAiming ? EquippedWeapon->AimingSpreadRecoverySpeed : EquippedWeapon->HipSpreadRecoverySpeed;
			CrosshairShootingFactor = FMath::FInterpTo(CrosshairShootingFactor, 0.f, DeltaTime, ShootingFactorRecoverySpeed);

			// 최종 추가 탄퍼짐 계산
			float CrosshairSpreadResult =
				+ CrosshairVelocityFactor		// 이동 속도에 비례해 +
				+ CrosshairInAirFactor			// 체공 중 +
				+ CrosshairShootingFactor		// 사격 시 +
				- CrosshairAimFactor;			// 조준 시 -
			
			EquippedWeapon->AddSpreadFactor = CrosshairSpreadResult;
			EquippedWeapon->AddSpreadFactor = FMath::FInterpTo(EquippedWeapon->AddSpreadFactor, 0.f, DeltaTime, ShootingFactorRecoverySpeed);

			// 최종 크로스헤어 스프레드 수치 계산 (기본 탄퍼짐 + 추가 탄퍼짐)
			CrosshairSpreadResult += EquippedWeapon->DefaultSpreadFactor / 100.f;

			// 음수가 되어 크로스헤어들이 서로를 가로지르는 현상을 방지
			HUDPackage.CrosshairSpread = FMath::Clamp(CrosshairSpreadResult, 0.f, 100.f);
			
			HUD->SetHUDPackage(HUDPackage);
		}
	}
}

void UCombatComponent::InterpFOV(float DeltaTime)
{
	if (EquippedWeapon == nullptr) return;

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
	if (Character == nullptr || EquippedWeapon == nullptr) return;
	
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
	if (Character == nullptr || WeaponToEquip == nullptr || CombatState != ECombatState::ECS_Unoccupied) return;
	
	if (EquippedWeapon != nullptr && SecondaryWeapon == nullptr)
	{
		EquipSecondaryWeapon(WeaponToEquip);
	}
	else
	{
		EquipPrimaryWeapon(WeaponToEquip);
	}
	
	Character->GetCharacterMovement()->bOrientRotationToMovement = false;
	Character->bUseControllerRotationYaw = true;
}

void UCombatComponent::ClientShouldChangeLocallyReloading_Implementation()
{
	bLocallyReloading = false;
}

void UCombatComponent::EquipPrimaryWeapon(AWeapon* WeaponToEquip)
{
	if (WeaponToEquip == nullptr) return;
	
	DropEquippedWeapon();
	
	EquippedWeapon = WeaponToEquip;
	EquippedWeapon->SetOwner(Character);
	EquippedWeapon->SetWeaponState(EWeaponState::EWS_Equipped);
	AttachActorToRightHand(EquippedWeapon);
	PlayEquipWeaponSound(EquippedWeapon);
	
	EquippedWeapon->SetHUDAmmo();
	UpdateCarriedAmmo();
	ReloadEmptyWeapon();

	MulticastBindFireTimer(EquippedWeapon, true);
}

void UCombatComponent::EquipSecondaryWeapon(AWeapon* WeaponToEquip)
{
	if (WeaponToEquip == nullptr) return;
	
	SecondaryWeapon = WeaponToEquip;
	SecondaryWeapon->SetWeaponState(EWeaponState::EWS_EquippedSecondary);
	AttachActorToBackpack(WeaponToEquip);
	
	PlayEquipWeaponSound(SecondaryWeapon);
	SecondaryWeapon->SetOwner(Character);

	MulticastBindFireTimer(SecondaryWeapon, false);
}

void UCombatComponent::MulticastBindFireTimer_Implementation(AWeapon* WeaponToBind, bool bShouldBind)
{
	if (WeaponToBind == nullptr) return;
	
	if (bShouldBind)
	{
		WeaponToBind->OnFireTimerFinished.BindUObject(this, &UCombatComponent::HandleFireTimerFinished);
	}
	else
	{
		WeaponToBind->OnFireTimerFinished.Unbind();
	}
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
	if (Character == nullptr || !Character->GetMesh() || ActorToAttach == nullptr) return;
	
	if (const USkeletalMeshSocket* HandSocket = Character->GetMesh()->GetSocketByName(FName("RightHandSocket")))
	{
		HandSocket->AttachActor(ActorToAttach, Character->GetMesh());
	}
}

void UCombatComponent::AttachActorToLeftHand(AActor* ActorToAttach)
{
	if (Character == nullptr || !Character->GetMesh() || ActorToAttach == nullptr || EquippedWeapon == nullptr) return;
	
	bool bUsePistolSocket =
		EquippedWeapon->GetWeaponType() == EWeaponType::EWT_Pistol
		|| EquippedWeapon->GetWeaponType() == EWeaponType::EWT_SubmachineGun;

	FName SocketName = bUsePistolSocket ? FName("PistolSocket") : FName("LeftHandSocket");
	if (const USkeletalMeshSocket* HandSocket = Character->GetMesh()->GetSocketByName(SocketName))
	{
		HandSocket->AttachActor(ActorToAttach, Character->GetMesh());
	}
}

void UCombatComponent::AttachActorToBackpack(AActor* ActorToAttach)
{
	if (Character == nullptr || !Character->GetMesh() || ActorToAttach == nullptr) return;

	if (const USkeletalMeshSocket* BackpackSocket = Character->GetMesh()->GetSocketByName(FName("BackpackSocket")))
	{
		BackpackSocket->AttachActor(ActorToAttach, Character->GetMesh());
	}
}

void UCombatComponent::UpdateCarriedAmmo()
{
	if (EquippedWeapon == nullptr) return;
	
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

void UCombatComponent::PlayEquipWeaponSound(AWeapon* WeaponToEquip)
{
	if (Character && EquippedWeapon && WeaponToEquip->EquipSound)
	{
		UGameplayStatics::PlaySoundAtLocation(
			this,
			WeaponToEquip->EquipSound,
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
		HandleReload();
		bLocallyReloading = true;
	}
}

void UCombatComponent::ServerReload_Implementation()
{
	if (Character == nullptr || EquippedWeapon == nullptr) return;

	// 상태 변경 후 애니메이션 재생 함수 호출
	CombatState = ECombatState::ECS_Reloading;
	if (!Character->IsLocallyControlled())
	{
		HandleReload();
	}
}

void UCombatComponent::HandleReload()
{
	if (Character)
	{	
		Character->PlayReloadMontage();
	}
}

void UCombatComponent::FinishReloading()
{
	if (Character == nullptr) return;

	bLocallyReloading = false;
	
	if (Character->HasAuthority())
	{
		CombatState = ECombatState::ECS_Unoccupied;
		UpdateAmmoValues();
	}
	
	// 좌클릭 누르고 있는 상태면 사격
	if (Character->IsLocallyControlled() && bFireButtonPressed)
	{
		Fire();
	}
}

void UCombatComponent::SwapWeapons()
{
	if (CombatState != ECombatState::ECS_Unoccupied || Character == nullptr) return;
	
	Character->PlaySwapMontage();
	Character->bFinishedSwapping = false;
	CombatState = ECombatState::ECS_SwappingWeapons;
}

void UCombatComponent::FinishSwapAttachWeapons()
{
	AWeapon* TempWeapon = EquippedWeapon;
	EquippedWeapon = SecondaryWeapon;
	SecondaryWeapon = TempWeapon;
	
	EquippedWeapon->SetWeaponState(EWeaponState::EWS_Equipped);
	AttachActorToRightHand(EquippedWeapon);
	PlayEquipWeaponSound(EquippedWeapon);
	EquippedWeapon->SetHUDAmmo();
	UpdateCarriedAmmo();
	MulticastBindFireTimer(EquippedWeapon, true);
	
	SecondaryWeapon->SetWeaponState(EWeaponState::EWS_EquippedSecondary);
	AttachActorToBackpack(SecondaryWeapon);
	MulticastBindFireTimer(SecondaryWeapon, false);
}

void UCombatComponent::FinishSwap()
{
	if (Character)
	{
		Character->bFinishedSwapping = true;
		if (Character->HasAuthority())
		{
			CombatState = ECombatState::ECS_Unoccupied;
		}
	}
}

void UCombatComponent::UpdateAmmoValues()
{
	if (Character == nullptr || EquippedWeapon == nullptr) return;
	
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
	if (Character == nullptr || EquippedWeapon == nullptr) return;

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

	bool bNeedMoreReload = EquippedWeapon->IsFull() || CarriedAmmo == 0;
	JumpToShotgunMoreReload(!bNeedMoreReload);
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
			bLocallyReloading = false;
		}
	}
}

int32 UCombatComponent::AmountToReload()
{
	if (EquippedWeapon == nullptr) return 0;

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
	if (Grenades == 0 || CombatState != ECombatState::ECS_Unoccupied || EquippedWeapon == nullptr) return;
	
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
	if (Grenades == 0) return;
	
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

	if (EquippedWeapon && EquippedWeapon->GetWeaponType() == EWeaponType::EWT_Shotgun)
	{
		bool bJumpToShotgunEnd = 
			CombatState == ECombatState::ECS_Reloading &&
			CarriedAmmo == 0;
		JumpToShotgunMoreReload(!bJumpToShotgunEnd);
	}
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
		if (Character && !Character->IsLocallyControlled())
		{
			// 패킷을 보낼 때 이미 재생하므로, 여기선 자신의 캐릭터가 아닌 경우만 재생한다.
			HandleReload();
		}
		break;
	case ECombatState::ECS_ThrowingGrenade:
		if (Character && !Character->IsLocallyControlled())
		{
			Character->PlayThrowGrenadeMontage();
			AttachActorToLeftHand(EquippedWeapon);
			ShowAttachedGrenade(true);
		}
		break;
	case ECombatState::ECS_SwappingWeapons:
		if (Character && !Character->IsLocallyControlled())
		{
			//Character->PlaySwapMontage();
		}
		break;
	}
}

void UCombatComponent::OnRep_EquippedWeapon()
{
	if (Character && EquippedWeapon)
	{
		EquippedWeapon->SetWeaponState(EWeaponState::EWS_Equipped);
		AttachActorToRightHand(EquippedWeapon);
		PlayEquipWeaponSound(EquippedWeapon);
		
		Character->GetCharacterMovement()->bOrientRotationToMovement = false;
		Character->bUseControllerRotationYaw = true;
		EquippedWeapon->SetHUDAmmo();
	}
}

void UCombatComponent::OnRep_SecondaryWeapon()
{
	if (Character && SecondaryWeapon)
	{
		SecondaryWeapon->SetWeaponState(EWeaponState::EWS_EquippedSecondary);
		AttachActorToBackpack(SecondaryWeapon);
		PlayEquipWeaponSound(SecondaryWeapon);
	}
}

bool UCombatComponent::CanFire()
{
	if (EquippedWeapon == nullptr) return false;

	if (!EquippedWeapon->IsEmpty() && EquippedWeapon->bCanFire && CombatState == ECombatState::ECS_Reloading && EquippedWeapon->GetWeaponType() == EWeaponType::EWT_Shotgun)
	{
		// 샷건 장전 중엔 예외로 사격 가능
		bLocallyReloading = false;
		return true;
	}

	if (bLocallyReloading) return false;

	return !EquippedWeapon->IsEmpty() && EquippedWeapon->bCanFire && CombatState == ECombatState::ECS_Unoccupied;
}

void UCombatComponent::HandleFireTimerFinished()
{
	// FireDelay만큼의 시간 후, 아직 마우스를 누르고 있으면서 연사 무기인 경우 다시 Fire가 실행
	if (bFireButtonPressed && EquippedWeapon->bAutomatic)
	{
		Fire();
	}

	ReloadEmptyWeapon();
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

bool UCombatComponent::ShouldSwapWeapons()
{
	// 샷건은 예외로 장전 중에도 변경 가능
	return (
		(EquippedWeapon != nullptr
		&& SecondaryWeapon != nullptr
		&& CombatState == ECombatState::ECS_Unoccupied)
		||
		(EquippedWeapon != nullptr
		&& SecondaryWeapon != nullptr
		&& CombatState != ECombatState::ECS_ThrowingGrenade
		&& EquippedWeapon->GetWeaponType() == EWeaponType::EWT_Shotgun)
		);
}