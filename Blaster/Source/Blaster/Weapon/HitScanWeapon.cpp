#include "HitScanWeapon.h"

#include "Engine/SkeletalMeshSocket.h"
#include "Blaster/Character/BlasterCharacter.h"
#include "Kismet/GameplayStatics.h"
#include "Particles/ParticleSystem.h"
#include "Particles/ParticleSystemComponent.h"
#include "Sound/SoundCue.h"
#include "Kismet/KismetMathLibrary.h"
#include "WeaponTypes.h"

#include "DrawDebugHelpers.h"

void AHitScanWeapon::Fire(const FVector& HitTarget)
{
	Super::Fire(HitTarget);

	APawn* OwnerPawn = Cast<APawn>(GetOwner());
	if (OwnerPawn == nullptr)
	{
		return;
	}
	
	const USkeletalMeshSocket* MuzzleFlashSocket = GetWeaponMesh()->GetSocketByName("MuzzleFlash");
	if (MuzzleFlashSocket)
	{
		FTransform SocketTransform = MuzzleFlashSocket->GetSocketTransform(GetWeaponMesh());
		FVector Start = SocketTransform.GetLocation();

		// 라인 트레이스 시작
		FHitResult FireHit;
		WeaponTraceHit(Start, HitTarget, FireHit);

		// 맞은 액터가 캐릭터인 경우 데미지 적용
		ABlasterCharacter* BlasterCharacter = Cast<ABlasterCharacter>(FireHit.GetActor());
		AController* InstigatorController = OwnerPawn->GetController();
		if (BlasterCharacter && InstigatorController && HasAuthority())
		{
			UGameplayStatics::ApplyDamage(
			BlasterCharacter,
			Damage,
			InstigatorController,
			this,
			UDamageType::StaticClass()
			);
		}
		PlayFX(FireHit, SocketTransform);
	}
}

void AHitScanWeapon::WeaponTraceHit(const FVector& TraceStart, const FVector& HitTarget, FHitResult& OutHit)
{
	UWorld* World = GetWorld();
	if (World)
	{
		FVector End = bUseScatter ? TraceEndWithScatter(TraceStart, HitTarget) : TraceStart + (HitTarget - TraceStart) * 1.25f;
		
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

FVector AHitScanWeapon::TraceEndWithScatter(const FVector& TraceStart, const FVector& HitTarget)
{
	// 총구에서 DistanceToSphere만큼 떨어진 SphereRadius를 반지름으로 하는 가상의 구체 생성
	// 해당 구체 안에서 랜덤한 점을 찍어 총구로부터 해당 점까지의 벡터를 계산
	FVector ToTargetNormalized = (HitTarget - TraceStart).GetSafeNormal();
	FVector SphereCenter = TraceStart + ToTargetNormalized * DistanceToSphere;
	float ResultSpreadFactor = DefaultSpreadFactor + SpreadFactor * 50.f;
	ResultSpreadFactor = FMath::Clamp(ResultSpreadFactor, 0.f, 100.f);
	FVector RandVec = UKismetMathLibrary::RandomUnitVector() * FMath::FRandRange(0.f, ResultSpreadFactor);
	FVector EndLoc = SphereCenter + RandVec;
	FVector ToEndLoc = EndLoc - TraceStart;

	// 총구로부터 랜덤으로 찍힌 점을 향한 80000길이의 벡터 계산
	constexpr float TraceLength = TRACE_LENGTH;
	return FVector(TraceStart + ToEndLoc * TraceLength / ToEndLoc.Size());
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