#include "Projectile.h"

#include "Components/BoxComponent.h"
#include "Kismet/GameplayStatics.h"
#include "Particles/ParticleSystem.h"
#include "Sound/SoundCue.h"
#include "Blaster/Blaster.h"
#include "NiagaraFunctionLibrary.h"
#include "ProjectileBullet.h"
#include "GameFramework/ProjectileMovementComponent.h"

AProjectile::AProjectile()
{
	PrimaryActorTick.bCanEverTick = true;
	bReplicates = true;

	CollisionBox = CreateDefaultSubobject<UBoxComponent>(TEXT("CollisionBox"));
	SetRootComponent(CollisionBox);
	CollisionBox->SetCollisionObjectType(ECC_WorldDynamic);
	CollisionBox->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
	CollisionBox->SetCollisionResponseToAllChannels(ECR_Ignore);
	CollisionBox->SetCollisionResponseToChannel(ECC_Visibility, ECR_Block);
	CollisionBox->SetCollisionResponseToChannel(ECC_WorldStatic, ECR_Block);
	CollisionBox->SetCollisionResponseToChannel(ECC_SkeletalMesh, ECR_Block);

	ProjectileMovementComponent = CreateDefaultSubobject<UProjectileMovementComponent>(TEXT("ProjectileMovementComponent"));
	ProjectileMovementComponent->bRotationFollowsVelocity = true;
	ProjectileMovementComponent->SetIsReplicated(true);
}

#if WITH_EDITOR
void AProjectile::PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent)
{
	Super::PostEditChangeProperty(PropertyChangedEvent);

	// 값이 변경된 변수가 멤버 변수 InitialSpeed인지 확인하고, 그 값을 ProjectileMovementComponent에 반영
	FName PropertyName = PropertyChangedEvent.Property != nullptr ? PropertyChangedEvent.Property->GetFName() : NAME_None;
	if (PropertyName == GET_MEMBER_NAME_CHECKED(AProjectile, InitialSpeed))
	{
		if (ProjectileMovementComponent)
		{
			ProjectileMovementComponent->InitialSpeed = InitialSpeed;
			ProjectileMovementComponent->MaxSpeed = InitialSpeed;
		}
	}
}
#endif

void AProjectile::BeginPlay()
{
	Super::BeginPlay();

	if (Tracer)
	{
		TracerComponent = UGameplayStatics::SpawnEmitterAttached(
			Tracer,
			CollisionBox,
			FName(),
			GetActorLocation(),
			GetActorRotation(),
			EAttachLocation::KeepWorldPosition
		);
	}

	StartBlockTimer();

	if (HasAuthority())
	{
		CollisionBox->OnComponentHit.AddDynamic(this, &ThisClass::OnHit);
	}

	// 투사체 경로 예측 세팅
	FPredictProjectilePathParams PathParams;
	PathParams.bTraceWithChannel = true;
	PathParams.bTraceWithCollision = true;
	PathParams.DrawDebugTime = 5.f;
	PathParams.DrawDebugType = EDrawDebugTrace::ForDuration;
	PathParams.LaunchVelocity = GetActorForwardVector() * InitialSpeed;
	PathParams.MaxSimTime = 4.f;
	PathParams.ProjectileRadius = 5.f;
	PathParams.SimFrequency = 30.f;
	PathParams.StartLocation = GetActorLocation();
	PathParams.TraceChannel = ECollisionChannel::ECC_Visibility;
	PathParams.ActorsToIgnore.Add(this);
	
	FPredictProjectilePathResult PathResults;
	// 투사체 경로 예측 함수
	UGameplayStatics::PredictProjectilePath(this, PathParams, PathResults);
}

void AProjectile::StartDestroyTimer()
{
	// DestroyTime 이후 Destroy 되도록 타이머 지정
	GetWorldTimerManager().SetTimer(
		DestroyTimer,
		this,
		&ThisClass::DestroyTimerFinished,
		DestroyTime
		);
}

void AProjectile::DestroyTimerFinished()
{
	Destroy();
}

void AProjectile::OnHit(UPrimitiveComponent* HitComp,
                        AActor* OtherActor,
                        UPrimitiveComponent* OtherComp,
                        FVector NormalImpulse,
                        const FHitResult& Hit)
{
	if (OtherActor->IsA(APawn::StaticClass()) && DefaultImpactParticle && HitCharacterImpactParticle)
	{
		DefaultImpactParticle = HitCharacterImpactParticle;
	}

	Destroy();
}

void AProjectile::AddVelocity(FVector Velocity)
{
	if (ProjectileMovementComponent)
	{
		ProjectileMovementComponent->Velocity += Velocity;
	}
}

void AProjectile::StartBlockTimer()
{
	// 발사한 폰과 충돌하지 않기 위한 Ignore 설정
	if (AActor* MyInstigator = GetInstigator())
	{
		CollisionBox->IgnoreActorWhenMoving(MyInstigator, true);
	}
	
	// BlockTime 이후 발사한 폰과 충돌할 수 있도록 타이머 지정
	GetWorldTimerManager().SetTimer(
		BlockTimer,
		this,
		&ThisClass::BlockTimerFinished,
		BlockTime
		);
}

void AProjectile::BlockTimerFinished()
{
	// 지정된 시간 이후 해당 투사체는 발사한 폰과 다시 충돌할 수 있게 됨
	if (AActor* MyInstigator = GetInstigator())
	{
		CollisionBox->IgnoreActorWhenMoving(MyInstigator, false);
	}
}

void AProjectile::SpawnTrailSystem()
{
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
}

void AProjectile::ExplodeDamage()
{
	APawn* FiringPawn = GetInstigator();

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
				DamageInnerRadius, // 최대 데미지 반경
				DamageOuterRadius, // 최소 데미지 반경
				1.f, // 데미지 감소 비율
				UDamageType::StaticClass(), // 데미지 타입 클래스
				TArray<AActor*>(), // 데미지를 받지 않을 액터
				this, // 데미지 유발자
				FiringController // InstigatorController
				);
		}
	}
}

void AProjectile::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
}

void AProjectile::Destroyed()
{
	Super::Destroyed();
	
	MulticastPlayFX();
}

void AProjectile::MulticastPlayFX_Implementation()
{
	if (DefaultImpactParticle)
	{
		UGameplayStatics::SpawnEmitterAtLocation(GetWorld(), DefaultImpactParticle, GetActorTransform());
	}
	if (ImpactSound)
	{
		UGameplayStatics::PlaySoundAtLocation(this, ImpactSound, GetActorLocation());
	}
}
