#include "ProjectileBullet.h"

#include "Blaster/Weapon/DamageType/BaseDamageType.h"
#include "Blaster/Character/BlasterCharacter.h"
#include "Blaster/PlayerController/BlasterPlayerController.h"
#include "Blaster/BlasterComponents/LagCompensationComponent.h"
#include "Kismet/GameplayStatics.h"

void AProjectileBullet::OnHit(UPrimitiveComponent* HitComp,
                              AActor* OtherActor,
                              UPrimitiveComponent* OtherComp,
                              FVector NormalImpulse,
                              const FHitResult& Hit)
{
	if (ABlasterCharacter* InstigatorCharacter = Cast<ABlasterCharacter>(GetInstigator()))
	{
		if (ABlasterPlayerController* InstigatorController = Cast<ABlasterPlayerController>(InstigatorCharacter->Controller))
		{
			if (InstigatorCharacter->HasAuthority()) // 서버가 들어오는 분기
			{
				// 즉시 데미지 적용 후 return
				UGameplayStatics::ApplyDamage(OtherActor, Damage, InstigatorController, this, UBaseDamageType::StaticClass());
				Super::OnHit(HitComp, OtherActor, OtherComp, NormalImpulse, Hit);
				return;
			}

			// 클라이언트인 경우 서버에 데미지 요청
			ABlasterCharacter* HitCharacter = Cast<ABlasterCharacter>(OtherActor);
			if (InstigatorCharacter->GetLagCompensation() && InstigatorCharacter->IsLocallyControlled() && HitCharacter)
			{
				InstigatorCharacter->GetLagCompensation()->ProjectileServerScoreRequest(
					HitCharacter,
					TraceStart,
					InitialVelocity,
					InstigatorController->GetServerTime() - InstigatorController->SingleTripTime,
					Damage
					);
			}
		}
	}
	
	Super::OnHit(HitComp, OtherActor, OtherComp, NormalImpulse, Hit);
}
