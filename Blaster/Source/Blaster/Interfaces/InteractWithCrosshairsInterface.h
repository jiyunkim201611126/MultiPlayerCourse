#pragma once

#include "CoreMinimal.h"
#include "UObject/Interface.h"
#include "InteractWithCrosshairsInterface.generated.h"

UINTERFACE(MinimalAPI)
class UInteractWithCrosshairsInterface : public UInterface
{
	GENERATED_BODY()
};

/**
 * 크로스헤어 기능과의 상호작용을 가능하게 하는 인터페이스입니다.
 *
 * 참고: 이 클래스는 인터페이스로서 상호작용 기능을 위한 청사진 역할을 하며, 
 * 자체적으로 동작을 직접 구현하지 않습니다. 구체적인 로직은 상속받는 클래스에서 
 * 작성해야 합니다.
 */

class BLASTER_API IInteractWithCrosshairsInterface
{
	GENERATED_BODY()
	// Add interface functions to this class. This is the class that will be inherited to implement this interface.
public:
};
