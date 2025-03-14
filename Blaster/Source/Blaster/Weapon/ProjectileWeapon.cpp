#include "ProjectileWeapon.h"

#include "Engine/SkeletalMeshSocket.h"
#include "Projectiles/Projectile.h"

void AProjectileWeapon::Fire(const FVector& HitTarget)
{
	// 탄환 소모
	Super::Fire(HitTarget);

	if (!HasAuthority())
	{
		return;
	}

	APawn* InstigatorPawn = Cast<APawn>(GetOwner());

	// 총구쪽 Socket
	const USkeletalMeshSocket* MuzzleFlashSocket = GetWeaponMesh()->GetSocketByName(FName("MuzzleFlash"));
	if (MuzzleFlashSocket)
	{
		// LineTrace 시작 Transform(== 총구쪽 Socket의 Transform)
		const FTransform SocketTransform = MuzzleFlashSocket->GetSocketTransform(GetWeaponMesh());
		
		const FVector ToTarget = HitTarget - SocketTransform.GetLocation();
		const FRotator TargetRotation = ToTarget.Rotation();
		if (ProjectileClass && InstigatorPawn)
		{
			// ProjectileClass를 기반으로 한 액터 스폰
			FActorSpawnParameters SpawnParams;
			SpawnParams.Owner = this;
			SpawnParams.Instigator = InstigatorPawn;
			if (UWorld* World = GetWorld())
			{
				AProjectile* Projectile = World->SpawnActor<AProjectile>(
					ProjectileClass,
					SocketTransform.GetLocation(),
					TargetRotation,
					SpawnParams
				);
				Projectile->Damage = Damage;
			}
		}
	}
}
