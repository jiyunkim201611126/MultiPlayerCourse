#include "HitScanWeapon.h"

#include "Engine/SkeletalMeshSocket.h"
#include "Blaster/Character/BlasterCharacter.h"
#include "Kismet/GameplayStatics.h"
#include "Particles/ParticleSystem.h"
#include "Particles/ParticleSystemComponent.h"
#include "Sound/SoundCue.h"
#include "Blaster/BlasterComponents/LagCompensationComponent.h"

#include "DrawDebugHelpers.h"
#include "Blaster/PlayerController/BlasterPlayerController.h"

void AHitScanWeapon::Fire(const FVector& HitTarget)
{
	Super::Fire(HitTarget);

	APawn* OwnerPawn = Cast<APawn>(GetOwner());
	if (OwnerPawn == nullptr) return;
	
	const USkeletalMeshSocket* MuzzleFlashSocket = GetWeaponMesh()->GetSocketByName("MuzzleFlash");
	if (MuzzleFlashSocket)
	{
		FTransform SocketTransform = MuzzleFlashSocket->GetSocketTransform(GetWeaponMesh());
		FVector Start = SocketTransform.GetLocation();

		// 라인 트레이스 시작
		FHitResult FireHit;
		WeaponTraceHit(Start, HitTarget, FireHit);

		// 캐릭터에게 적중 시 데미지 프레임워크로 들어간다
		ABlasterCharacter* HitCharacter = Cast<ABlasterCharacter>(FireHit.GetActor());
		AController* InstigatorController = OwnerPawn->GetController();
		BlasterOwnerCharacter = BlasterOwnerCharacter == nullptr ? Cast<ABlasterCharacter>(OwnerPawn) : BlasterOwnerCharacter;
		if (HitCharacter && InstigatorController && BlasterOwnerCharacter)
		{
			if (HasAuthority() && !bUseServerSideRewind) // SSR이 꺼진 클라이언트와 서버가 들어오는 분기
			{
				UGameplayStatics::ApplyDamage(
				HitCharacter,
				Damage,
				InstigatorController,
				this,
				UDamageType::StaticClass()
				);
			}
			// 클라이언트의 경우 아직은 적중이 확실하지 않으며, 적중했다고 주장하는 상태
			// 따라서 서버에 검증을 요청한다
			if (!HasAuthority() && bUseServerSideRewind) // SSR이 켜진 클라이언트가 들어오는 분기
			{
				BlasterOwnerController = BlasterOwnerController == nullptr ? Cast<ABlasterPlayerController>(InstigatorController) : BlasterOwnerController;
				if (BlasterOwnerController && BlasterOwnerCharacter->GetLagCompensation())
				{
					// 서버에 데미지를 요청
					BlasterOwnerCharacter->GetLagCompensation()->ServerScoreRequest(
						HitCharacter,		// 데미지를 받을 캐릭터
						Start,				// 라인 트레이스 시작 지점
						HitTarget,			// 라인 트레이스 종료 지점
						BlasterOwnerController->GetServerTime() - BlasterOwnerController->SingleTripTime // 적중한 시간
						);
				}
			}
		}
		PlayFX(FireHit, SocketTransform);
	}
}

void AHitScanWeapon::WeaponTraceHit(const FVector& TraceStart, const FVector& HitTarget, FHitResult& OutHit)
{
	UWorld* World = GetWorld();
	if (World)
	{
		FVector End = TraceStart + (HitTarget - TraceStart) * 1.25f;
		
		World->LineTraceSingleByChannel(
			OutHit,
			TraceStart,
			End,
			ECC_Visibility
			);

		// 적중 결과에 따른 총알 궤적 파티클 위치 변경
		FVector BeamEnd = End;
		if (OutHit.bBlockingHit)
		{
			BeamEnd = OutHit.ImpactPoint;
		}
		
		// 총알 궤적 파티클
		if (BeamParticles)
		{
			UParticleSystemComponent* Beam = UGameplayStatics::SpawnEmitterAtLocation(
				World,
				BeamParticles,
				TraceStart,
				FRotator::ZeroRotator,
				true
				);
			if (Beam)
			{
				Beam->SetVectorParameter(FName("Target"), BeamEnd);
			}
		}
	}
}

void AHitScanWeapon::PlayFX(const FHitResult& FireHit, const FTransform& SocketTransform)
{
	// 적중한 액터와 관계 없이 FX 재생
	if (ImpactParticles)
	{
		UGameplayStatics::SpawnEmitterAtLocation(
			GetWorld(),
			ImpactParticles,
			FireHit.ImpactPoint,
			FireHit.ImpactNormal.Rotation()
			);
	}
	if (HitSound)
	{
		UGameplayStatics::PlaySoundAtLocation(
			this,
			HitSound,
			FireHit.ImpactPoint
			);
	}

	// 총기의 애니메이션 에셋이 없는 경우 따로 FX를 재생
	if (MuzzleFlash)
	{
		UGameplayStatics::SpawnEmitterAtLocation(
			GetWorld(),
			MuzzleFlash,
			SocketTransform
			);
	}
	if (FireSound)
	{
		UGameplayStatics::PlaySoundAtLocation(
			this,
			FireSound,
			GetActorLocation()
			);
	}
}