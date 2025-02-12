#pragma once

#include "CoreMinimal.h"
#include "GameFramework/DamageType.h"
#include "BaseDamageType.generated.h"

UCLASS()
class BLASTER_API UBaseDamageType : public UDamageType
{
	GENERATED_BODY()
	
public:
	virtual void ApplyDamageTypeEffect(AActor* DamagedActor, AController* Instigator) const;
};
