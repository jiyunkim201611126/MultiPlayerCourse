#include "ProjectileBullet.h"

#include "DamageType/BurnDamageType.h"
#include "GameFramework/Character.h"
#include "Kismet/GameplayStatics.h"

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
			UGameplayStatics::ApplyDamage(OtherActor, Damage, InstigatorController, this, UBurnDamageType::StaticClass());
		}
	}
	Super::OnHit(HitComp, OtherActor, OtherComp, NormalImpulse, Hit);
}
