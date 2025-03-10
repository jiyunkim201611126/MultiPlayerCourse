#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "PickupSpawnPoint.generated.h"

UCLASS()
class BLASTER_API APickupSpawnPoint : public AActor
{
	GENERATED_BODY()

public:
	APickupSpawnPoint();
	virtual void Tick(float DeltaTime) override;

protected:
	virtual void BeginPlay() override;

	UPROPERTY(EditAnywhere)
	TArray<TSubclassOf<class APickup>> PickupClasses;

	UPROPERTY()
	APickup* SpawnedPickup = nullptr;
	
	UFUNCTION()
	void StartSpawnPickupTimer(AActor* DestroyedActor);
	void SpawnPickupTimerFinished();
	void SpawnPickup();

private:
	FTimerHandle SpawnPickupTimer;

	UPROPERTY(EditAnywhere)
	float SpawnPickupTime = 10.f;
};
