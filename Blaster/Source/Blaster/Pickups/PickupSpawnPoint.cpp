#include "PickupSpawnPoint.h"

#include "Pickup.h"

APickupSpawnPoint::APickupSpawnPoint()
{
	PrimaryActorTick.bCanEverTick = true;
	bReplicates = true;
}

void APickupSpawnPoint::BeginPlay()
{
	Super::BeginPlay();
	StartSpawnPickupTimer((AActor*)nullptr);
}

void APickupSpawnPoint::StartSpawnPickupTimer(AActor* DestroyedActor)
{
	GetWorldTimerManager().SetTimer(
		SpawnPickupTimer,
		this,
		&ThisClass::SpawnPickupTimerFinished,
		SpawnPickupTime);
}

void APickupSpawnPoint::SpawnPickupTimerFinished()
{
	if (HasAuthority())
	{
		SpawnPickup();
	}
}

void APickupSpawnPoint::SpawnPickup()
{
	int32 NumPickupClasses = PickupClasses.Num();
	if (NumPickupClasses > 0)
	{
		int32 Selection = FMath::RandRange(0, NumPickupClasses - 1);
		SpawnedPickup = GetWorld()->SpawnActor<APickup>(PickupClasses[Selection], GetActorTransform());

		if (HasAuthority() && SpawnedPickup)
		{
			SpawnedPickup->OnDestroyed.AddDynamic(this, &ThisClass::StartSpawnPickupTimer);
		}
	}
}

void APickupSpawnPoint::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
}
