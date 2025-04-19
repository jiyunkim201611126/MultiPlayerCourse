#pragma once

#include "CoreMinimal.h"
#include "Blaster/BlasterTypes/Team.h"
#include "UObject/Interface.h"
#include "InteractWithCrosshairsInterface.generated.h"

UINTERFACE(MinimalAPI)
class UInteractWithCrosshairsInterface : public UInterface
{
	GENERATED_BODY()
};

/**
 * 크로스헤어와 상호작용을 가능하게 하는 인터페이스입니다.
 */

class BLASTER_API IInteractWithCrosshairsInterface
{
	GENERATED_BODY()
	
public:
	virtual ETeam GetTeam();
};
