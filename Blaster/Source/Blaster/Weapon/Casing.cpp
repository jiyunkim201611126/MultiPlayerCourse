#include "Casing.h"

#include "Kismet/GameplayStatics.h"
#include "Sound/SoundCue.h"

ACasing::ACasing()
{
	PrimaryActorTick.bCanEverTick = false;

	CasingMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("CasingMesh"));
	SetRootComponent(CasingMesh);

	// 탄피가 조작에 영향을 주지 않도록 충돌 설정
	CasingMesh->SetCollisionResponseToChannel(ECollisionChannel::ECC_Camera, ECollisionResponse::ECR_Ignore);
	CasingMesh->SetCollisionResponseToChannel(ECollisionChannel::ECC_Pawn, ECollisionResponse::ECR_Ignore);
	// 물리 시뮬레이션
	CasingMesh->SetSimulatePhysics(true);
	CasingMesh->SetEnableGravity(true);
	// HitEvent 활성화
	CasingMesh->SetNotifyRigidBodyCollision(true);
	ShellEjectionImpulse = 5.f;
}

void ACasing::BeginPlay()
{
	Super::BeginPlay();

	// 오른쪽 방향으로 살짝 랜덤하게 탄피 배출
	const float RandomYaw = FMath::FRandRange(-10.f, 20.f);
	const float RandomRoll = FMath::FRandRange(-10.f, 10.f);
	CasingMesh->AddWorldRotation(FRotator(0, RandomYaw, RandomRoll));
	CasingMesh->AddImpulse(GetActorForwardVector() * ShellEjectionImpulse);
	
	CasingMesh->OnComponentHit.AddDynamic(this, &ThisClass::OnHit);
}

void ACasing::OnHit(UPrimitiveComponent* HitComp,
	AActor* OtherActor,
	UPrimitiveComponent* OtherComp,
	FVector NormalImpulse,
	const FHitResult& Hit)
{
	// 더이상 HitEvent를 발생시키지 않도록 설정, 해당 함수가 연속 호출되는 걸 방지
	CasingMesh->SetNotifyRigidBodyCollision(false);
	
	if (ShellSound)
	{
		UGameplayStatics::PlaySoundAtLocation(this, ShellSound, GetActorLocation());
	}

	// 5초 후 Destroy하도록 람다식 타이머에 바인드
	FTimerHandle TimerHandle;
	GetWorld()->GetTimerManager().SetTimer(TimerHandle, [this]()
	{
		Destroy();
	}, 5.0f, false);
}
