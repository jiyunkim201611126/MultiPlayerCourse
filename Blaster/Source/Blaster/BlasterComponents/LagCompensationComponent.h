#pragma once

#include "CoreMinimal.h"
#include "Blaster/Character/BlasterCharacter.h"
#include "Components/ActorComponent.h"
#include "LagCompensationComponent.generated.h"

class AProjectile;

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

USTRUCT(BlueprintType)
struct FRocketServerSideRewindResult
{
	GENERATED_BODY()

	UPROPERTY()
	bool bHitConfirmed;

	UPROPERTY()
	bool bHeadShot;

	UPROPERTY()
	FVector_NetQuantize HitLocation;
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
	
	/**
	 * 클라이언트가 서버에 적중 사실 확인과 함께 데미지를 요청하는 함수
	 * 
	 * @param HitCharacter 공격에 의해 잠재적으로 적중된 캐릭터
	 * @param TraceStart 적중 여부를 판단하기 위해 사용될 라인 트레이스의 시작 지점
	 * @param HitLocation 적중이 발생했을 것으로 예상되는 지점
	 * @param HitTime 적중이 발생했을 것으로 믿어지는 시점
	 */
	UFUNCTION(Server, Reliable) // 히트스캔용
	void ServerScoreRequest(
		ABlasterCharacter* HitCharacter,
		const FVector_NetQuantize& TraceStart,
		const FVector_NetQuantize& HitLocation,
		float HitTime);

	// 위 함수와 같은 용도, 샷건용
	UFUNCTION(Server, Reliable)
	void ShotgunServerScoreRequest(
		const TArray<ABlasterCharacter*>& HitCharacters,
		const FVector_NetQuantize& TraceStart,
		const TArray<FVector_NetQuantize>& HitLocations,
		float HitTime);

	// 위 함수와 같은 용도, 투사체용
	UFUNCTION(Server, Reliable)
	void ProjectileServerScoreRequest(
		ABlasterCharacter* HitCharacter,
		const FVector_NetQuantize& TraceStart,
		const FVector_NetQuantize100& InitialVelocity,
		float HitTime,
		class AProjectileBullet* Projectile);

	// 위 함수와 같은 용도, 로켓용
	// 폭발형 무기는 클라이언트와 서버의 적중 결과가 다른 경우 클라이언트쪽에서 크게 이상해 보일 수 있지만, 그냥 구현해보고 싶어서 구현함
	UFUNCTION(Server, Reliable)
	void RocketServerScoreRequest(
		AActor* HitActor,
		const FVector_NetQuantize& TraceStart,
		const FVector_NetQuantize100& InitialVelocity,
		float HitTime,
		AProjectile* Rocket);
	
	/**
	 * 서버 상태를 되돌려 특정 시점에서 주어진 공격에 의해 캐릭터가 맞았는지 판단합니다
	 * 이 함수는 저장된 프레임 패키지를 사용하여 히트박스의 위치를 보간하고 잠재적 충돌을 평가합니다
	 *
	 * @param HitCharacter 공격에 의해 잠재적으로 적중된 캐릭터
	 * @param TraceStart 적중 여부를 판단하기 위해 사용될 라인 트레이스의 시작 지점
	 * @param HitLocation 적중이 발생했을 것으로 예상되는 지점
	 * @param HitTime 적중이 발생했을 것으로 믿어지는 시점
	 * @return 적중 여부와 헤드샷 여부를 나타내는 결과를 반환
	 */
	FServerSideRewindResult ServerSideRewind( // 히트스캔용
		ABlasterCharacter* HitCharacter,
		const FVector_NetQuantize& TraceStart,
		const FVector_NetQuantize& HitLocation,
		float HitTime);

	// 위 함수와 같은 용도, 샷건용
	FShotgunServerSideRewindResult ShotgunServerSideRewind(
		const TArray<ABlasterCharacter*>& HitCharacters,
		const FVector_NetQuantize& TraceStart,
		const TArray<FVector_NetQuantize>& HitLocations,
		float HitTime);

	// 위 함수와 같은 용도, 투사체용
	FServerSideRewindResult ProjectileServerSideRewind(
		ABlasterCharacter* HitCharacter,
		const FVector_NetQuantize& TraceStart,
		const FVector_NetQuantize100& InitialVelocity,
		float HitTime);

	// 위 함수와 같은 용도, 로켓용
	FRocketServerSideRewindResult RocketServerSideRewind(
		ABlasterCharacter* HitCharacter,
		const FVector_NetQuantize& TraceStart,
		const FVector_NetQuantize100& InitialVelocity,
		float HitTime);
	
protected:
	virtual void BeginPlay() override;
	
	// BoxComponent의 현 상태를 FrameHistory에 담는 함수, Tick에서 호출
	void SaveFramePackage();
	
	// 이 컴포넌트를 가진 Character의 HitBox 역할을 하는 BoxComponent들의 위치를 저장하는 함수
	void SaveFramePackage(FFramePackage& Package);

	// HitBox를 DebugBox로 보여주는 함수
	void ShowFramePackage(const FFramePackage& Package, const FColor& Color);

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

	// 모든 BoxComponent의 Collision을 끄는 함수
	void OffCollisionHitBoxes(ABlasterCharacter* HitCharacter);

	// SSR후 적중 결과를 반환하는 함수, 히트스캔용
	FServerSideRewindResult ConfirmHit(
		const FFramePackage& Package,
		const FVector_NetQuantize& TraceStart,
		const FVector_NetQuantize& HitLocation);

	// SSR후 적중 결과를 반환하는 함수, 샷건용
	FShotgunServerSideRewindResult ShotgunConfirmHit(
		const TArray<FFramePackage>& FramePackages,
		const FVector_NetQuantize& TraceStart,
		const TArray<FVector_NetQuantize>& HitLocations);

	// SSR후 적중 결과를 반환하는 함수, 투사체용
	FServerSideRewindResult ProjectileConfirmHit(
		const FFramePackage& Package,
		const FVector_NetQuantize& TraceStart,
		const FVector_NetQuantize100& InitialVelocity);

	// SSR후 적중 결과를 반환하는 함수, 로켓용
	FRocketServerSideRewindResult RocketConfirmHit(
		const FFramePackage& Package,
		const FVector_NetQuantize& TraceStart,
		const FVector_NetQuantize100& InitialVelocity);
	
private:
	UPROPERTY()
	ABlasterCharacter* Character;

	UPROPERTY()
	class ABlasterPlayerController* Controller;

	TDoubleLinkedList<FFramePackage> FrameHistory;

	UPROPERTY(EditAnywhere)
	float MaxRecordTime = 4.f;
};
