#include "Shotgun.h"

#include "Engine/SkeletalMeshSocket.h"
#include "Blaster/Character/BlasterCharacter.h"
#include "Kismet/GameplayStatics.h"
#include "Kismet/KismetMathLibrary.h"

void AShotgun::FireShotgun(const TArray<FVector_NetQuantize>& HitTargets)
{
	AWeapon::Fire(FVector::ZeroVector);
	
	APawn* OwnerPawn = Cast<APawn>(GetOwner());
	if (OwnerPawn == nullptr)
	{
		return;
	}
	
	if (const USkeletalMeshSocket* MuzzleFlashSocket = GetWeaponMesh()->GetSocketByName("MuzzleFlash"))
	{
		const FTransform SocketTransform = MuzzleFlashSocket->GetSocketTransform(GetWeaponMesh());
		const FVector Start = SocketTransform.GetLocation();
		AController* InstigatorController = OwnerPawn->GetController();

		// Key는 적중 캐릭터, Value는 적중 회수
		TMap<ABlasterCharacter*, int32> HitMap;

		for (FVector_NetQuantize HitTarget : HitTargets)
		{
			FHitResult FireHit;
			WeaponTraceHit(Start, HitTarget, FireHit);
			ABlasterCharacter* BlasterCharacter = Cast<ABlasterCharacter>(FireHit.GetActor());

			// 캐릭터에게 적중 시 Map을 통해 캐릭터마다 몇 개의 펠릿을 맞았는지 기록
			// 이 동작은 서버와 클라이언트 모두가 함
			if (BlasterCharacter)
			{
				if (HitMap.Contains(BlasterCharacter))
				{
					HitMap[BlasterCharacter]++;
				}
				else
				{
					HitMap.Emplace(BlasterCharacter, 1);
				}

				// 적중 액터과 관계 없이 FX 재생
				PlayFX(FireHit, SocketTransform);
			}

			// 적중 결과를 순회하며 펠릿 개수만큼 데미지 적용
			for (auto HitPair : HitMap)
			{
				if (HitPair.Key && InstigatorController && HasAuthority())
				{
					UGameplayStatics::ApplyDamage(
					HitPair.Key,
					Damage * HitPair.Value,
					InstigatorController,
					this,
					UDamageType::StaticClass()
					);
				}
			}
		}
	}
}

void AShotgun::ShotgunTraceEndWithScatter(const FVector& HitTarget, TArray<FVector_NetQuantize>& HitTargets)
{
	// TraceEndWithScatter를 반복문으로 호출해도 되지만, 아래 수많은 지역변수들이 계속해서 재선언되므로 성능을 위해 재작성
	const USkeletalMeshSocket* MuzzleFlashSocket = GetWeaponMesh()->GetSocketByName("MuzzleFlash");
	if (MuzzleFlashSocket == nullptr)
	{
		return;
	}
	
	const FTransform SocketTransform = MuzzleFlashSocket->GetSocketTransform(GetWeaponMesh());
	const FVector TraceStart = SocketTransform.GetLocation();
	
	const FVector ToTargetNormalized = (HitTarget - TraceStart).GetSafeNormal();
	const FVector SphereCenter = TraceStart + ToTargetNormalized * DistanceToSphere;
	float ResultSpreadFactor = DefaultSpreadFactor + AddSpreadFactor * 50.f;
	ResultSpreadFactor = FMath::Clamp(ResultSpreadFactor, 0.f, 100.f);
	
	for (uint32 i = 0; i < NumberOfPellets; i++)
	{
		// 랜덤 수치만 반복문에서 사용
		const FVector RandVec = UKismetMathLibrary::RandomUnitVector() * FMath::FRandRange(0.f, ResultSpreadFactor);
		const FVector EndLoc = SphereCenter + RandVec;
		FVector ToEndLoc = EndLoc - TraceStart;

		constexpr float TraceLength = TRACE_SHOTGUN_LENGTH;
		ToEndLoc = TraceStart + ToEndLoc * TraceLength / ToEndLoc.Size();
		
		HitTargets.Add(ToEndLoc);
	}
}
