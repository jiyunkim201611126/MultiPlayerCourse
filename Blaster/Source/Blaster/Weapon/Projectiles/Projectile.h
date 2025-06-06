#pragma once

#include "CoreMinimal.h"
#include "Blaster/Interfaces/CauseDamageInterface.h"
#include "GameFramework/Actor.h"
#include "Projectile.generated.h"

UCLASS()
class BLASTER_API AProjectile : public AActor, public IICauseDamageInterface
{
	GENERATED_BODY()
	
public:	
	AProjectile();
	virtual void Tick(float DeltaTime) override;
	virtual void Destroyed() override;
	
	float Damage = 0.f;

	UPROPERTY(EditAnywhere)
	float DamageInnerRadius = 200.f;
	UPROPERTY(EditAnywhere)
	float DamageOuterRadius = 500.f;

	float HeadShotDamageModifier = 1.f;
	float TeammateDamageModifier = 1.f;

	/**
	 * SSR 구현을 위한 변수
	 */

	bool bUseServerSideRewind = false;

	// 2단 추진 로켓 구현을 위해 블루프린트도 열어둠
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FVector_NetQuantize TraceStart;
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FVector_NetQuantize100 InitialVelocity;

	UPROPERTY(EditAnywhere)
	float InitialSpeed = 15000.f;

	virtual void StartDestroyTimer();

#if WITH_EDITOR // 에디터에서만 호출됨
	virtual void PostEditChangeProperty(struct FPropertyChangedEvent& PropertyChangedEvent) override;
#endif

protected:
	virtual void BeginPlay() override;
	void DestroyTimerFinished();
	void SpawnTrailSystem();
	void ExplodeDamage();

	UFUNCTION()
	virtual void OnHit(
		UPrimitiveComponent* HitComp,
		AActor* OtherActor,
		UPrimitiveComponent* OtherComp,
		FVector NormalImpulse,
		const FHitResult& Hit);

	UFUNCTION(NetMulticast, Reliable)
	void MulticastPlayFX();
	
	UPROPERTY(EditAnywhere)
	class UBoxComponent* CollisionBox;

	UPROPERTY(VisibleAnywhere)
	UStaticMeshComponent* ProjectileMesh;
	
	// 발사체 움직임 관련 클래스
	UPROPERTY(VisibleAnywhere)
	class UProjectileMovementComponent* ProjectileMovementComponent;

	// 파티클들
	UPROPERTY(EditAnywhere)
	UParticleSystem* DefaultImpactParticle;
	
	UPROPERTY(EditAnywhere)
	UParticleSystem* HitCharacterImpactParticle;

	UPROPERTY(EditAnywhere)
	class USoundCue* ImpactSound;

	UPROPERTY(EditAnywhere)
	class UNiagaraSystem* TrailSystem;

	UPROPERTY()
	class UNiagaraComponent* TrailSystemComponent;

	UFUNCTION(BlueprintCallable)
	virtual void AddVelocity(FVector Velocity);
	
private:
	UPROPERTY(EditAnywhere)
	UParticleSystem* Tracer;
	
	UPROPERTY()
	UParticleSystemComponent* TracerComponent;
	
	FTimerHandle DestroyTimer;

	UPROPERTY(EditAnywhere)
	float DestroyTime = 3.f;

	// 발사 직후 충돌하지 않으나, 1초 후엔 충돌하기 위해 Timer 설정
	FTimerHandle BlockTimer;
	
	float BlockTime = 1.f;

	void StartBlockTimer();
	void BlockTimerFinished();
	
	/**
	 * ICauseDamage
	 */
public:
	virtual float GetTeammateDamageModifier() override { return TeammateDamageModifier; };
};
