#pragma once

#include "CoreMinimal.h"
#include "Blaster/Character/BlasterCharacter.h"
#include "Components/ActorComponent.h"
#include "LagCompensationComponent.generated.h"

USTRUCT(BlueprintType)
struct FBoxInformation
{
	GENERATED_BODY()

	UPROPERTY()
	FVector Location;

	UPROPERTY()
	FRotator Rotation;

	UPROPERTY()
	FVector BoxExtent;
};

USTRUCT(BlueprintType)
struct FFramePackage
{
	GENERATED_BODY()

	UPROPERTY()
	float Time;

	UPROPERTY()
	TMap<FName, FBoxInformation> HitBoxInfo;
};

USTRUCT(BlueprintType)
struct FServerSideRewindResult
{
	GENERATED_BODY()

	UPROPERTY()
	bool bHitConfirmed;

	UPROPERTY()
	bool bHeadShot;
};

UCLASS(ClassGroup=(Custom), meta=(BlueprintSpawnableComponent))
class BLASTER_API ULagCompensationComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	ULagCompensationComponent();
	friend class ABlasterCharacter;
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;
	void ShowFramePackage(const FFramePackage& Package, const FColor& Color);
	
	FServerSideRewindResult ServerSideRewind(
		ABlasterCharacter* HitCharacter,
		const FVector_NetQuantize& TraceStart,
		const FVector_NetQuantize& HitLocation,
		float HitTime);

	UFUNCTION(Server, Reliable)
	void ServerScoreRequest(
		ABlasterCharacter* HitCharacter,
		const FVector_NetQuantize& TraceStart,
		const FVector_NetQuantize& HitLocation,
		float HitTime,
		class AWeapon* DamageCauser);
	
protected:
	virtual void BeginPlay() override;
	
	// 이 컴포넌트를 가진 Character의 HitBox가 될 BoxComponent들의 위치를 저장하는 함수
	void SaveFramePackage(FFramePackage& Package);

	// HitTime을 사이에 두고 있는 2개의 FramePackage의 그 사이, HitTime의 FramePackage를 구하는 함수
	FFramePackage InterpBetweenFrames(const FFramePackage& OlderFrame, const FFramePackage& YoungerFrame, float HitTime);

	// SSR후 적중 결과를 반환하는 함수
	FServerSideRewindResult ConfirmHit(
		const FFramePackage& Package,
		ABlasterCharacter* HitCharacter,
		const FVector_NetQuantize& TraceStart,
		const FVector_NetQuantize& HitLocation);
	
	// 현재 프레임의 BoxPosition을 잠시 저장하는 함수
	void CacheBoxPositions(ABlasterCharacter* HitCharacter, FFramePackage& OutFramePackage);
	
	// 현재 프레임의 BoxComponent들을 특정 위치로 옮기는 함수 (라인 트레이스를 통해 Hit 판정을 보기 위해 되감기한다)
	void MoveBoxes(ABlasterCharacter* HitCharacter, const FFramePackage& Package);
	
	// BoxComponent를 잠시 특정 위치로 옮겼으므로, 라인 트레이스가 끝나면 다시 되돌린다.
	void ResetHitBoxes(ABlasterCharacter* HitCharacter, const FFramePackage& Package);
	
	// 캐릭터 Mesh의 CollisionEnabled를 조정하는 함수
	void EnableCharacterMeshCollision(ABlasterCharacter* HitCharacter, ECollisionEnabled::Type CollisionEnabled);

	// BoxComponent의 현 상태를 FrameHistory에 담는 함수, Tick에서 호출
	void SaveFramePackage();
	
private:
	UPROPERTY()
	ABlasterCharacter* Character;

	UPROPERTY()
	class ABlasterPlayerController* Controller;

	TDoubleLinkedList<FFramePackage> FrameHistory;

	UPROPERTY(EditAnywhere)
	float MaxRecordTime = 4.f;
};
