#pragma once

#include "CoreMinimal.h"
#include "BaseDamageType.h"
#include "BurnDamageType.generated.h"

UCLASS()
class BLASTER_API UBurnDamageType : public UBaseDamageType
{
	GENERATED_BODY()

public:
	UPROPERTY(BlueprintReadOnly, Category="Damage")
	float BurnDamage = 3.f;

	UPROPERTY(BlueprintReadOnly, Category="Damage")
	int NumberOfTimesToBurn = 5; // 총 n번 데미지
	
	UPROPERTY(BlueprintReadOnly, Category="Damage")
	float BurnDelay = 2.f; // n초마다 데미지

	// 타이머 추적용
	UPROPERTY(BlueprintReadOnly, Category="Damage")
	mutable FTimerHandle BurnTimer;
	
	UPROPERTY(BlueprintReadOnly, Category="Damage")
	mutable int BurnCount = 0; // 현재 데미지 준 회수 추적
	
	virtual void ApplyDamageTypeEffect(AActor* DamagedActor, AController* Instigator) override;
	void BurnTimerStart(AActor* DamagedActor, AController* Instigator);
	void BurnTimerFinished(AActor* DamagedActor, AController* Instigator);
};
