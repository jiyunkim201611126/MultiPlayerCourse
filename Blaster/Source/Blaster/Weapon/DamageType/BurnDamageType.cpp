#include "BurnDamageType.h"

#include "Kismet/GameplayStatics.h"

void UBurnDamageType::ApplyDamageTypeEffect(AActor* DamagedActor, AController* Instigator)
{
    if (DamagedActor && Instigator)
    {
        BurnTimerStart(DamagedActor, Instigator);
    }
}

void UBurnDamageType::BurnTimerStart(AActor* DamagedActor, AController* Instigator)
{
    if (UWorld* World = DamagedActor->GetWorld())
    {
        World->GetTimerManager().SetTimer(
            BurnTimer,
            [this, DamagedActor, Instigator]()
            {
                BurnTimerFinished(DamagedActor, Instigator);
            },
            BurnDelay,
            false
        );
    }
}

void UBurnDamageType::BurnTimerFinished(AActor* DamagedActor, AController* Instigator)
{
    // 여기선 속성 없이 데미지 부여
    UGameplayStatics::ApplyDamage(DamagedActor, BurnDamage, Instigator, Instigator, UDamageType::StaticClass());

    // Count 세면서 다시 타이머 시작
    BurnCount++;
    if (BurnCount < NumberOfTimesToBurn)
    {
        BurnTimerStart(DamagedActor, Instigator);
    }
    else
    {
        MarkAsGarbage();
    }
}
