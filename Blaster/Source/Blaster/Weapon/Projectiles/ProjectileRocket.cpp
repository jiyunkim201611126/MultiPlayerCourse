#include "ProjectileRocket.h"
#include "Kismet/GameplayStatics.h"
#include "NiagaraComponent.h"
#include "NiagaraSystemInstanceController.h"
#include "Components/BoxComponent.h"
#include "Sound/SoundCue.h"
#include "Components/AudioComponent.h"

AProjectileRocket::AProjectileRocket()
{
	ProjectileMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("RocketMesh"));
	ProjectileMesh->SetupAttachment(RootComponent);
	ProjectileMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
}

void AProjectileRocket::BeginPlay()
{
	Super::BeginPlay();

	// 이 클래스는 충돌 즉시 Destroy되는 게 아니기 때문에, 충돌 시 파티클과 사운드 재생을 위해 클라이언트도 OnHit을 호출
	if (!HasAuthority())
	{
		CollisionBox->OnComponentHit.AddDynamic(this, &ThisClass::OnHit);
	}

	SpawnTrailSystem();
	
	// 날아가는 소리 재생
	if (ProjectileLoop && LoopingSoundAttenuation)
	{
		ProjectileLoopComponent = UGameplayStatics::SpawnSoundAttached(
			ProjectileLoop,
			GetRootComponent(),
			FName(),
			GetActorLocation(),
			EAttachLocation::KeepWorldPosition,
			false,
			1.f,
			1.f,
			0.f,
			LoopingSoundAttenuation,
			(USoundConcurrency*)nullptr,
			false
			);
	}
}

void AProjectileRocket::OnHit(UPrimitiveComponent* HitComp, AActor* OtherActor, UPrimitiveComponent* OtherComp,
                              FVector NormalImpulse, const FHitResult& Hit)
{
	ExplodeDamage();

	StartDestroyTimer();
	
	if (OtherActor->IsA(APawn::StaticClass()) && DefaultImpactParticle && HitCharacterImpactParticle)
	{
		DefaultImpactParticle = HitCharacterImpactParticle;
	}
	if (HasAuthority())
	{
		MulticastPlayFX();
	}
}

void AProjectileRocket::Destroyed()
{
	Super::Super::Destroyed();
}

void AProjectileRocket::MulticastPlayFX_Implementation()
{
	// 즉시 Destroy되면 안 되기 때문에 Super::OnHit을 호출할 수 없음
	// 따라서 파티클과 사운드를 재생하는 함수들을 여기서 호출
	if (DefaultImpactParticle)
	{
		UGameplayStatics::SpawnEmitterAtLocation(GetWorld(), DefaultImpactParticle, GetActorTransform());
	}
	if (ImpactSound)
	{
		UGameplayStatics::PlaySoundAtLocation(this, ImpactSound, GetActorLocation());
	}
	// 메쉬 숨기고 콜리전 끄기
	if (ProjectileMesh)
	{
		ProjectileMesh->SetVisibility(false);
	}
	if (CollisionBox)
	{
		CollisionBox->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	}
	// Trail 파티클 비활성화
	if (TrailSystemComponent && TrailSystemComponent->GetSystemInstanceController())
	{
		TrailSystemComponent->GetSystemInstanceController()->Deactivate();
	}
	// 날아가는 소리 중단
	if (ProjectileLoopComponent && ProjectileLoopComponent->IsPlaying())
	{
		ProjectileLoopComponent->Stop();
	}
}