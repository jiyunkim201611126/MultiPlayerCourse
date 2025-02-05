#include "CombatComponent.h"

#include "Blaster/Character/BlasterCharacter.h"
#include "Blaster/Weapon/Weapon.h"
#include "Engine/SkeletalMeshSocket.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Net/UnrealNetwork.h"
#include "Kismet/GameplayStatics.h"
#include "DrawDebugHelpers.h"
#include "Blaster/PlayerController/BlasterPlayerController.h"
#include "Blaster/HUD/BlasterHUD.h"

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
}

void UCombatComponent::BeginPlay()
{
	Super::BeginPlay();

	if (Character)
	{
		Character->GetCharacterMovement()->MaxWalkSpeed = BaseWalkSpeed;
	}
}

void UCombatComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	SetHUDCrosshairs(DeltaTime);
	if (Character && Character->IsLocallyControlled())
	{
		FHitResult HitResult;
		HitTarget = TraceUnderCrosshairs(HitResult);
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
			FHUDPackage HUDPackage;
			if (EquippedWeapon)
			{
				HUDPackage.CrosshairsCenter = EquippedWeapon->CrosshairsCenter;
				HUDPackage.CrosshairsLeft = EquippedWeapon->CrosshairsLeft;
				HUDPackage.CrosshairsRight = EquippedWeapon->CrosshairsRight;
				HUDPackage.CrosshairsTop = EquippedWeapon->CrosshairsTop;
				HUDPackage.CrosshairsBottom = EquippedWeapon->CrosshairsBottom;
			}
			else
			{
				HUDPackage.CrosshairsCenter = nullptr;
				HUDPackage.CrosshairsLeft = nullptr;
				HUDPackage.CrosshairsRight = nullptr;
				HUDPackage.CrosshairsTop = nullptr;
				HUDPackage.CrosshairsBottom = nullptr;
			}
			
			// 크로스헤어 스프레드 계산
			FVector2D WalkSpeedRange(0.f, Character->GetCharacterMovement()->MaxWalkSpeed);
			FVector2D VelocityMultiplierRange(0.f, 1.f);
			FVector Velocity = Character->GetVelocity();
			Velocity.Z = 0;
			
			CrosshairVelocityFactor = FMath::GetMappedRangeValueClamped(WalkSpeedRange, VelocityMultiplierRange, Velocity.Size());

			if (Character->GetCharacterMovement()->IsFalling())
			{
				CrosshairInAirFactor = FMath::FInterpTo(CrosshairInAirFactor, 2.25f, DeltaTime, 2.25f);
			}
			else
			{
				CrosshairInAirFactor = FMath::FInterpTo(CrosshairInAirFactor, 0.f, DeltaTime, 30.f);
			}
			
			HUDPackage.CrosshairSpread = CrosshairVelocityFactor + CrosshairInAirFactor;
			
			HUD->SetHUDPackage(HUDPackage);
		}
	}
}

void UCombatComponent::SetAiming(bool bIsAiming)
{
	// 일단 바로 변경, 아직 다른 클라이언트는 반영되지 않음
	bAiming = bIsAiming;

	// 클라이언트인 경우 Aim을 서버에 요청
	ServerSetAiming(bIsAiming);
	
	if (Character)
	{
		Character->GetCharacterMovement()->MaxWalkSpeed = bIsAiming ? AimWalkSpeed : BaseWalkSpeed;
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

void UCombatComponent::OnRep_EquippedWeapon()
{
	if (EquippedWeapon && Character)
	{
		Character->GetCharacterMovement()->bOrientRotationToMovement = false;
		Character->bUseControllerRotationYaw = true;
	}
}

void UCombatComponent::FireButtonPressed(bool bPressed)
{
	// 일단 바로 변경, 아직 다른 클라이언트는 반영되지 않음
	bFireButtonPressed = bPressed;

	if (bFireButtonPressed)
	{
		// 사격 시작
		FHitResult HitResult;
		LocalFire(TraceUnderCrosshairs(HitResult));
	}
}

void UCombatComponent::LocalFire(const FVector_NetQuantize& TraceHitTarget)
{
	// 착용 중인 무기가 없는 경우 바로 리턴
	if (EquippedWeapon == nullptr)
	{
		return;
	}
	
	if (Character && Character->IsLocallyControlled())
	{
		// 캐릭터 사격 애니메이션 재생
		Character->PlayFireMontage(bAiming);

		// 무기 사격 함수 호출, 다른 클라이언트는 아직 반영되지 않음
		EquippedWeapon->Fire(TraceHitTarget);

		// 서버에 사격 요청
		ServerFire(TraceHitTarget);
	}
}

void UCombatComponent::ServerFire_Implementation(const FVector_NetQuantize& TraceHitTarget)
{
	// 해당 함수는 ServerFire는 클라이언트가 서버에 요청하는 함수
	// MulticastFire는 서버가 실질적으로 클라이언트들에게 함수를 원격 호출해주는 함수
	// 관여하는 변수 중 Replicated 속성이 없어 함수를 한 번 거쳐 구현
	MulticastFire(TraceHitTarget);
}

void UCombatComponent::MulticastFire_Implementation(const FVector_NetQuantize& TraceHitTarget)
{
	// 서버가 클라이언트들에게 함수를 원격 호출해주는 함수
	// 착용 중인 무기가 없는 경우 바로 리턴
	if (EquippedWeapon == nullptr)
	{
		return;
	}

	// 내 캐릭터인 경우 이미 사격을 시작하고 호출되었기 때문에, 내 캐릭터가 아닌 경우만 사격 함수 호출
	if (Character && !Character->IsLocallyControlled())
	{
		Character->PlayFireMontage(bAiming);
		EquippedWeapon->Fire(TraceHitTarget);
	}
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
	const FVector Start = CrosshairWorldPosition;
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

void UCombatComponent::EquipWeapon(AWeapon* WeaponToEquip)
{
	if (Character == nullptr || WeaponToEquip == nullptr)
	{
		return;
	}

	// 무기 장착, EquippedWeapon의 UPROPERTY로 인해 OnRep_EquippedWeapon이 자동 실행
	EquippedWeapon = WeaponToEquip;
	EquippedWeapon->SetWeaponState(EWeaponState::EWS_Equipped);

	// Attach Weapon to RightHandSocket
	if (const USkeletalMeshSocket* HandSocket = Character->GetMesh()->GetSocketByName(FName("RightHandSocket")))
	{
		HandSocket->AttachActor(EquippedWeapon, Character->GetMesh());
	}

	EquippedWeapon->SetOwner(Character);
}