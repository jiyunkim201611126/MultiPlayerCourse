#pragma once

#include "CoreMinimal.h"
#include "Weapon.h"
#include "HitScanWeapon.generated.h"

UCLASS()
class BLASTER_API AHitScanWeapon : public AWeapon
{
	GENERATED_BODY()

public:
	virtual void Fire(const FVector& HitTarget) override;

protected:
	// 탄퍼짐을 적용한 수치로 라인 트레이스의 End 벡터를 계산하는 함수
	FVector TraceEndWithScatter(const FVector& TraceStart, const FVector& HitTarget);
	// 기본 End 벡터 혹은 위에서 계산된 End 벡터를 기반으로 라인 트레이스
	void WeaponTraceHit(const FVector& TraceStart, const FVector& HitTarget, FHitResult& OutHit);
	// FX 재생
	void PlayFX(const FHitResult& FireHit, const FTransform& SocketTransform);
	
	UPROPERTY(EditAnywhere)
	UParticleSystem* ImpactParticles;

	UPROPERTY(EditAnywhere)
	USoundCue* HitSound;
	
private:
	UPROPERTY(EditAnywhere)
	UParticleSystem* BeamParticles;

	UPROPERTY(EditAnywhere)
	UParticleSystem* MuzzleFlash;

	UPROPERTY(EditAnywhere)
	USoundCue* FireSound;
	
	/**
	 * Trace end with scatter
	 */
	// 탄퍼짐의 기준이 되는 구체의 거리. 클수록 탄퍼짐이 좁아짐
	UPROPERTY(EditAnywhere, Category = "Weapon Scatter")
	float DistanceToSphere = 800.f;
};
