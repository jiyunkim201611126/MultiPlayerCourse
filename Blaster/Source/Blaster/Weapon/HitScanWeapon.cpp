#include "HitScanWeapon.h"

#include "Engine/SkeletalMeshSocket.h"
#include "Blaster/Character/BlasterCharacter.h"
#include "Kismet/GameplayStatics.h"
#include "Particles/ParticleSystem.h"
#include "Particles/ParticleSystemComponent.h"

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
		FVector End = Start + (HitTarget - Start) * 1.25f;
		FHitResult FireHit;
		
		// 라인 트레이스
		if (UWorld* World = GetWorld())
		{
			World->LineTraceSingleByChannel(
				FireHit,
				Start,
				End,
				ECC_Visibility
				);
			
			// 충돌과 관계 없이 궤적 파티클 생성을 위한 벡터
			FVector BeamEnd = End;
			if (FireHit.bBlockingHit)
			{
				// 충돌이 있는 경우 해당 위치로 바꿔줌
				BeamEnd = FireHit.ImpactPoint;

				// 충돌 시 로직, 데미지와 Impact 파티클
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
				if (ImpactParticles)
				{
					UGameplayStatics::SpawnEmitterAtLocation(
						World,
						ImpactParticles,
						FireHit.ImpactPoint,
						FireHit.ImpactNormal.Rotation()
						);
				}
			}

			// 총알 궤적 파티클
			if (BeamParticles)
			{
				UParticleSystemComponent* Beam = UGameplayStatics::SpawnEmitterAtLocation(
					World,
					BeamParticles,
					SocketTransform
					);
				if (Beam)
				{
					Beam->SetVectorParameter(FName("Target"), BeamEnd);
				}
			}
		}
	}
}
