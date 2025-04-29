#pragma once

#include "CoreMinimal.h"
#include "Weapon.h"
#include "Blaster/BlasterTypes/Team.h"
#include "Flag.generated.h"

UCLASS()
class BLASTER_API AFlag : public AWeapon
{
	GENERATED_BODY()

public:
	AFlag();
	
	virtual void Dropped() override;
	void ResetFlag();

protected:
	virtual void BeginPlay() override;
	virtual void OnConstruction(const FTransform& Transform) override;

	/**
	 * Weapon의 OnEquipped와 OnDropped는 bUseServerSideRewind에 대한 세팅이 들어가나,
	 * Flag는 해당 변수가 의미를 갖지 않으므로 생략하도록 override
	 */
	
	virtual void OnEquipped() override;
	virtual void OnDropped() override;

	virtual void OnSphereOverlap(
		UPrimitiveComponent* OverlappedComponent,
		AActor* OtherActor,
		UPrimitiveComponent* OtherComp,
		int32 OtherBodyIndex,
		bool bFromSweep,
		const FHitResult& SweepResult) override;
	
	virtual void OnSphereEndOverlap(
		UPrimitiveComponent* OverlappedComponent,
		AActor* OtherActor,
		UPrimitiveComponent* OtherComp,
		int32 OtherBodyIndex) override;

private:
	UPROPERTY(VisibleAnywhere)
	UStaticMeshComponent* FlagMesh;

	UPROPERTY(EditAnywhere)
	ETeam Team;
	
	// 제자리로 돌아갈 때 사용할 Transform
	FTransform InitialTransform;

public:
	FORCEINLINE ETeam GetTeam() const { return Team; }
	FORCEINLINE FTransform GetInitialTransform() const { return InitialTransform; }
};
