#include "ProjectileRocket.h"
#include "Kismet/GameplayStatics.h"
#include "NiagaraFunctionLibrary.h"
#include "NiagaraComponent.h"
#include "NiagaraSystemInstanceController.h"
#include "Components/BoxComponent.h"
#include "Sound/SoundCue.h"
#include "Components/AudioComponent.h"
#include "RocketMovementComponent.h"

AProjectileRocket::AProjectileRocket()
{
	RocketMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("RocketMesh"));
	RocketMesh->SetupAttachment(RootComponent);
	RocketMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);

	RocketMovementComponent = CreateDefaultSubobject<URocketMovementComponent>(TEXT("RocketMovementComponent"));
	RocketMovementComponent->bRotationFollowsVelocity = true;
	RocketMovementComponent->SetIsReplicated(true);
}

void AProjectileRocket::BeginPlay()
{
	Super::BeginPlay();

	// 이 클래스는 충돌 즉시 Destroy되는 게 아니기 때문에, 충돌 시 파티클과 사운드 재생을 위해 클라이언트도 OnHit을 호출
	if (!HasAuthority())
	{
		CollisionBox->OnComponentHit.AddDynamic(this, &ThisClass::OnHit);
	}

	// 로켓의 꼬리 부분에 Trail 파티클 스폰
	if (TrailSystem)
	{
		FVector Origin;
		FVector BoxExtent;
		GetActorBounds(true, Origin, BoxExtent);

		FVector TrailLocation = -FVector(BoxExtent.X, 0.f, 0.f);

		TrailSystemComponent = UNiagaraFunctionLibrary::SpawnSystemAttached(
			TrailSystem,
			GetRootComponent(),
			FName(),
			TrailLocation,
			GetActorRotation(),
			EAttachLocation::KeepRelativeOffset,
			false
			);
	}
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
	APawn* FiringPawn = GetInstigator();

	if (OtherActor == FiringPawn)
	{
		return;
	}

	// 데미지는 서버에서만 줄 수 있다
	if (FiringPawn && HasAuthority())
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

	// DestroyTime 이후 Destroy 되도록 타이머 지정
	GetWorldTimerManager().SetTimer(
		DestroyTimer,
		this,
		&AProjectileRocket::DestroyTimerFinished,
		DestroyTime
		);

	// 즉시 Destroy되면 안 되기 때문에 Super::OnHit을 호출할 수 없음
	// 따라서 파티클과 사운드를 재생하는 함수들을 여기서 호출
	if (OtherActor->IsA(APawn::StaticClass()) && DefaultImpactParticle && HitCharacterImpactParticle)
	{
		DefaultImpactParticle = HitCharacterImpactParticle;
	}
	if (DefaultImpactParticle)
	{
		UGameplayStatics::SpawnEmitterAtLocation(GetWorld(), DefaultImpactParticle, GetActorTransform());
	}
	if (ImpactSound)
	{
		UGameplayStatics::PlaySoundAtLocation(this, ImpactSound, GetActorLocation());
	}
	// 메쉬 숨기고 콜리전 끄기
	if (RocketMesh)
	{
		RocketMesh->SetVisibility(false);
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

void AProjectileRocket::DestroyTimerFinished()
{
	Destroy();
}

void AProjectileRocket::Destroyed()
{
	Super::Super::Destroyed();
}