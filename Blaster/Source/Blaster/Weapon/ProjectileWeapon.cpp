#include "ProjectileWeapon.h"

#include "Engine/SkeletalMeshSocket.h"
#include "Kismet/KismetMathLibrary.h"
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
		// 총구에서 HitTarget으로의 방향 계산. 탄퍼짐 있는 경우 SphereRadius에 비례해 랜덤 Rotator로 사격
		float AddFloat = DefaultSpreadFactor + SpreadFactor;
		AddFloat = FMath::Clamp(AddFloat, 0.0f, 100.0f);
		const FRotator RandomRotator = FRotator(FMath::FRandRange(-AddFloat, AddFloat),
			FMath::FRandRange(-AddFloat, AddFloat),
			FMath::FRandRange(-AddFloat, AddFloat));
		
		const FVector ToTarget = HitTarget - SocketTransform.GetLocation();
		const FRotator TargetRotation = bUseScatter ? ToTarget.Rotation() + RandomRotator : ToTarget.Rotation();
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
