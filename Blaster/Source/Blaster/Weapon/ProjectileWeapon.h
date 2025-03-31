#pragma once

#include "CoreMinimal.h"
#include "Weapon.h"
#include "ProjectileWeapon.generated.h"

UCLASS()
class BLASTER_API AProjectileWeapon : public AWeapon
{
	GENERATED_BODY()

public:
	virtual void Fire(const FVector& HitTarget) override;
	
private:
	// 서버만 스폰시키며, 클라이언트들에게 복제되는 액터
	UPROPERTY(EditAnywhere)
	TSubclassOf<class AProjectile> ProjectileClass;

	// 복제되지 않는 Projectile 액터, Locally Fire에 사용됨
	UPROPERTY(EditAnywhere)
	TSubclassOf<AProjectile> ServerSideRewindProjectileClass;

	UPROPERTY(EditAnywhere)
	bool bUseServerSideRewind = false;
};
