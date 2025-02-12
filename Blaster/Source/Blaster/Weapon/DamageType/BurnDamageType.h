#pragma once

#include "CoreMinimal.h"
#include "BaseDamageType.h"
#include "BurnDamageType.generated.h"

UCLASS()
class BLASTER_API UBurnDamageType : public UBaseDamageType
{
	GENERATED_BODY()

public:
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Damage")
	float BurnDamage = 3.f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Damage")
	int NumberOfTimesToBurn = 5; // 총 n번 데미지

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Damage")
	float BurnDelay = 2.f; // n초마다 데미지

	virtual void ApplyDamageTypeEffect(AActor* DamagedActor, AController* Instigator) const override;
	void BurnTimerStart(AActor* DamagedActor, AController* Instigator) const;
	void BurnTimerFinished(AActor* DamagedActor, AController* Instigator) const;
};
