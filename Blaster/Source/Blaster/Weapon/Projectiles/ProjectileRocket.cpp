#include "ProjectileRocket.h"
#include "Kismet/GameplayStatics.h"

AProjectileRocket::AProjectileRocket()
{
	RocketMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("RocketMesh"));
	RocketMesh->SetupAttachment(RootComponent);
	RocketMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
}

void AProjectileRocket::OnHit(UPrimitiveComponent* HitComp, AActor* OtherActor, UPrimitiveComponent* OtherComp,
                              FVector NormalImpulse, const FHitResult& Hit)
{
	APawn* FiringPawn = GetInstigator();
	if (FiringPawn)
	{
		AController* FiringController = FiringPawn->GetController();
		if (FiringController)
		{
			UGameplayStatics::ApplyRadialDamageWithFalloff(
				this, // 월드 객체
				Damage, // 최대 데미지
				10.f, // 최소 데미지
				GetActorLocation(), // 데미지 시작 지점
				200.f, // 최대 데미지 반경
				500.f, // 최소 데미지 반경
				1.f, // 데미지 감소 비율
				UDamageType::StaticClass(), // 데미지 타입 클래스
				TArray<AActor*>(), // 데미지를 받지 않을 액터
				this, // 데미지 유발자
				FiringController // InstigatorController
				);
		}
	}
	
	Super::OnHit(HitComp, OtherActor, OtherComp, NormalImpulse, Hit);
}
