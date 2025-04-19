#include "ProjectileWeapon.h"

#include "Engine/SkeletalMeshSocket.h"
#include "Projectiles/Projectile.h"

void AProjectileWeapon::Fire(const FVector& HitTarget)
{
	// 탄환 소모 및 탄피 스폰
	Super::Fire(HitTarget);

	APawn* InstigatorPawn = Cast<APawn>(GetOwner());

	// 총구쪽 Socket
	const USkeletalMeshSocket* MuzzleFlashSocket = GetWeaponMesh()->GetSocketByName(FName("MuzzleFlash"));
	UWorld* World = GetWorld();
	if (MuzzleFlashSocket && World)
	{
		// LineTrace 시작 Transform(== 총구쪽 Socket의 Transform)
		const FTransform SocketTransform = MuzzleFlashSocket->GetSocketTransform(GetWeaponMesh());
		const FVector ToTarget = HitTarget - SocketTransform.GetLocation();
		const FRotator TargetRotation = ToTarget.Rotation();
		
		// ProjectileClass를 기반으로 한 액터 스폰
		FActorSpawnParameters SpawnParams;
		SpawnParams.Owner = this;
		SpawnParams.Instigator = InstigatorPawn;
		
		AProjectile* SpawnedProjectile = nullptr;

		if (bUseServerSideRewind)
		{
			if (InstigatorPawn->HasAuthority()) // 서버가 들어오는 분기
			{
				if (InstigatorPawn->IsLocallyControlled()) // 서버가 발사 - Replicated Projectile 스폰, SSR 필요 없음
				{
					SpawnedProjectile = World->SpawnActor<AProjectile>(ServerSideRewindProjectileClass, SocketTransform.GetLocation(), TargetRotation, SpawnParams);
					SpawnedProjectile->bUseServerSideRewind = false;
				}
				else // 클라이언트가 발사 - Non-Replicated Projectile 스폰
				{
					SpawnedProjectile = World->SpawnActor<AProjectile>(ServerSideRewindProjectileClass, SocketTransform.GetLocation(), TargetRotation, SpawnParams);
					SpawnedProjectile->bUseServerSideRewind = true;
				}
			}
			else // 클라이언트가 들어오는 분기, SSR 사용
			{
				if (InstigatorPawn->IsLocallyControlled()) // 클라이언트 자신이 발사, Non-Replicated Projectile 스폰, SSR 사용 
				{
					SpawnedProjectile = World->SpawnActor<AProjectile>(ServerSideRewindProjectileClass, SocketTransform.GetLocation(), TargetRotation, SpawnParams);
					SpawnedProjectile->TraceStart = SocketTransform.GetLocation();
					SpawnedProjectile->InitialVelocity = SpawnedProjectile->GetActorForwardVector() * SpawnedProjectile->InitialSpeed;
					SpawnedProjectile->bUseServerSideRewind = true;
				}
				else // 다른 클라이언트가 발사, Non-Replicated Projectile 스폰
				{
					SpawnedProjectile = World->SpawnActor<AProjectile>(ServerSideRewindProjectileClass, SocketTransform.GetLocation(), TargetRotation, SpawnParams);
					SpawnedProjectile->bUseServerSideRewind = false;
				}
			}
		}
		else
		{
			if (InstigatorPawn->HasAuthority())
			{
				SpawnedProjectile = World->SpawnActor<AProjectile>(ProjectileClass, SocketTransform.GetLocation(), TargetRotation, SpawnParams);
				SpawnedProjectile->bUseServerSideRewind = false;
			}
		}
		SpawnedProjectile->Damage = Damage;
		SpawnedProjectile->HeadShotDamageModifier = HeadShotDamageModifier;
		SpawnedProjectile->TeammateDamageModifier = TeammateDamageModifier;
	}
}
