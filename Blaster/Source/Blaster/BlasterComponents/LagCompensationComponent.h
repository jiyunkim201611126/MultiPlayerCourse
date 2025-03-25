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

	UPROPERTY()
	ABlasterCharacter* Character;
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

USTRUCT(BlueprintType)
struct FShotgunServerSideRewindResult
{
	GENERATED_BODY()

	UPROPERTY()
	TMap<ABlasterCharacter*, uint32> HeadShots;
	
	UPROPERTY()
	TMap<ABlasterCharacter*, uint32> BodyShots;
};

/**
 * 네트워크 환경에서 플레이어의 지연 보정을 처리하는 구성 요소를 나타냅니다.
 * 서버는 주어진 시간에 슈팅자의 시점에 맞게 위치를 되감아
 * 빠르게 움직이는 플레이어의 적중 결과를 정확하게 계산할 수 있습니다.
 */

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
	
	// 이 컴포넌트를 가진 Character의 HitBox 역할을 하는 BoxComponent들의 위치를 저장하는 함수
	void SaveFramePackage(FFramePackage& Package);

	// 아래 함수들을 거의 모두 호출하며, HitTime에 HitCharacter가 어디 있었는지 최종 반환하는 함수
	FFramePackage GetFrameToCheck(ABlasterCharacter* HitCharacter, float HitTime);

	// HitTime을 사이에 두고 있는 2개의 FramePackage의 그 사이, HitTime의 FramePackage를 구하는 함수
	FFramePackage InterpBetweenFrames(const FFramePackage& OlderFrame, const FFramePackage& YoungerFrame, float HitTime);
	
	// 현재 프레임의 BoxPosition을 잠시 저장하는 함수
	void CacheBoxPositions(ABlasterCharacter* HitCharacter, FFramePackage& OutFramePackage);
	
	// 현재 프레임의 BoxComponent들을 특정 위치로 옮기는 함수 (라인 트레이스를 통해 Hit 판정을 보기 위해 되감기한다)
	void MoveBoxes(ABlasterCharacter* HitCharacter, const FFramePackage& Package);
	
	// BoxComponent를 잠시 특정 위치로 옮겼으므로, 라인 트레이스가 끝나면 다시 되돌린다.
	void ResetHitBoxes(ABlasterCharacter* HitCharacter, const FFramePackage& Package);
	
	// 캐릭터 Mesh의 CollisionEnabled를 조정하는 함수
	void EnableCharacterMeshCollision(ABlasterCharacter* HitCharacter, ECollisionEnabled::Type CollisionEnabled);

	// SSR후 적중 결과를 반환하는 함수
	FServerSideRewindResult ConfirmHit(
		const FFramePackage& Package,
		const FVector_NetQuantize& TraceStart,
		const FVector_NetQuantize& HitLocation);

	// BoxComponent의 현 상태를 FrameHistory에 담는 함수, Tick에서 호출
	void SaveFramePackage();
	
	/**
	 * Shotgun
	 */
	FShotgunServerSideRewindResult ShotgunServerSideRewind(
		const TArray<ABlasterCharacter*>& HitCharacters,
		const FVector_NetQuantize& TraceStart,
		const TArray<FVector_NetQuantize>& HitLocations,
		float HitTime);

	FShotgunServerSideRewindResult ShotgunConfirmHit(
		const TArray<FFramePackage>& FramePackages,
		const FVector_NetQuantize& TraceStart,
		const TArray<FVector_NetQuantize>& HitLocations
		);
	
private:
	UPROPERTY()
	ABlasterCharacter* Character;

	UPROPERTY()
	class ABlasterPlayerController* Controller;

	TDoubleLinkedList<FFramePackage> FrameHistory;

	UPROPERTY(EditAnywhere)
	float MaxRecordTime = 4.f;
};
