#pragma once

#include "CoreMinimal.h"
#include "UObject/Interface.h"
#include "ICauseDamage.generated.h"

UINTERFACE(MinimalAPI)
class UICauseDamage : public UInterface
{
	GENERATED_BODY()
};

class BLASTER_API IICauseDamage
{
	GENERATED_BODY()
	// Add interface functions to this class. This is the class that will be inherited to implement this interface.

public:
	float Damage = 0.f;
	
	float HeadShotDamageModifier = 2.f;
	
	float TeammateDamageModifier = 0.5f;
	
	FORCEINLINE float GetTeammateDamageModifier() const { return TeammateDamageModifier; }
};
