#include "RocketMovementComponent.h"

UProjectileMovementComponent::EHandleBlockingHitResult URocketMovementComponent::HandleBlockingHit(
	const FHitResult& Hit,
	float TimeTick,
	const FVector& MoveDelta,
	float& SubTickTimeRemaining)
{
	Super::HandleBlockingHit(Hit, TimeTick, MoveDelta, SubTickTimeRemaining);
	return EHandleBlockingHitResult::AdvanceNextSubstep;
}

void URocketMovementComponent::HandleImpact(const FHitResult& Hit, float TimeSlice, const FVector& MoveDelta)
{
	// 부모 함수를 호출하지 않는 것으로, 로켓을 발사한 플레이어와 충돌해도 계속 날아갈 수 있도록 함
	//Super::HandleImpact(Hit, TimeSlice, MoveDelta);
}
