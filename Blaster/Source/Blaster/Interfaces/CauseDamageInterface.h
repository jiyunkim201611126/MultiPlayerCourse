#pragma once

#include "CoreMinimal.h"
#include "UObject/Interface.h"
#include "CauseDamageInterface.generated.h"

UINTERFACE(MinimalAPI)
class UICauseDamageInterface : public UInterface
{
	GENERATED_BODY()
};

class BLASTER_API IICauseDamageInterface
{
	GENERATED_BODY()

public:
	virtual float GetTeammateDamageModifier();
};
