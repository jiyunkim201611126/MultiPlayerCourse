#include "BurnDamageType.h"

#include "Kismet/GameplayStatics.h"

void UBurnDamageType::ApplyDamageTypeEffect(AActor* DamagedActor, AController* Instigator) const
{
    if (DamagedActor && Instigator)
    {
        BurnTimerStart(DamagedActor, Instigator);
    }
}

void UBurnDamageType::BurnTimerStart(AActor* DamagedActor, AController* Instigator) const
{
    if (const UWorld* World = DamagedActor->GetWorld())
    {
        for (int BurnCount = 0; BurnCount <= NumberOfTimesToBurn; BurnCount++)
        {
            FTimerHandle BurnTimer;
            World->GetTimerManager().SetTimer(
                BurnTimer,
                [this, DamagedActor, Instigator]()
                {
                    BurnTimerFinished(DamagedActor, Instigator);
                },
                BurnDelay * BurnCount,
                false
            );
        }
    }
}

void UBurnDamageType::BurnTimerFinished(AActor* DamagedActor, AController* Instigator) const
{
    UGameplayStatics::ApplyDamage(DamagedActor, BurnDamage, Instigator, Instigator, UDamageType::StaticClass());
}
