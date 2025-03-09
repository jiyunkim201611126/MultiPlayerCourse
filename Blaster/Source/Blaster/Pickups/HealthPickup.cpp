#include "HealthPickup.h"

#include "Blaster/Character/BlasterCharacter.h"
#include "Blaster/BlasterComponents/BuffComponent.h"

void AHealthPickup::OnSphereOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor,
                                    UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult)
{
	Super::OnSphereOverlap(OverlappedComponent, OtherActor, OtherComp, OtherBodyIndex, bFromSweep, SweepResult);

	if (ABlasterCharacter* BlasterCharacter = Cast<ABlasterCharacter>(OtherActor))
	{
		if (BlasterCharacter->GetHealth() >= BlasterCharacter->GetMaxHealth())
		{
			return;
		}
		
		if (UBuffComponent* Buff = BlasterCharacter->GetBuff())
		{
			Buff->Heal(HealAmount, HealingTime);
		}
	}
	Destroy();
}