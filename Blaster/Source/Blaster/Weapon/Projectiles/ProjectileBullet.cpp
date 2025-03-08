#include "ProjectileBullet.h"

#include "Blaster/Weapon/DamageType/BaseDamageType.h"
#include "GameFramework/Character.h"
#include "Kismet/GameplayStatics.h"

AProjectileBullet::AProjectileBullet()
{
}

void AProjectileBullet::OnHit(UPrimitiveComponent* HitComp,
                              AActor* OtherActor,
                              UPrimitiveComponent* OtherComp,
                              FVector NormalImpulse,
                              const FHitResult& Hit)
{
	if (ACharacter* InstigatorCharacter = Cast<ACharacter>(GetInstigator()))
	{
		if (AController* InstigatorController = InstigatorCharacter->Controller)
		{
			UGameplayStatics::ApplyDamage(OtherActor, Damage, InstigatorController, this, UBaseDamageType::StaticClass());
		}
	}
	Super::OnHit(HitComp, OtherActor, OtherComp, NormalImpulse, Hit);
}
