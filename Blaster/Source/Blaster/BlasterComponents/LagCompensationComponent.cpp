#include "LagCompensationComponent.h"

#include "Blaster/Character/BlasterCharacter.h"
#include "Components/BoxComponent.h"
#include "Kismet/GameplayStatics.h"
#include "Blaster/Weapon/Weapon.h"
#include "Blaster/Blaster.h"
#include "Blaster/Weapon/Projectiles/ProjectileBullet.h"
#include "DrawDebugHelpers.h"
#include "Blaster/GameMode/BlasterGameMode.h"

ULagCompensationComponent::ULagCompensationComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
}

void ULagCompensationComponent::BeginPlay()
{
	Super::BeginPlay();
}

void ULagCompensationComponent::ServerScoreRequest_Implementation(
	ABlasterCharacter* HitCharacter,
	const FVector_NetQuantize& TraceStart,
	const FVector_NetQuantize& HitLocation,
	float HitTime)
{
	
	FServerSideRewindResult Confirm = ServerSideRewind(HitCharacter, TraceStart, HitLocation, HitTime);

	if (Character && Character->GetController() && HitCharacter && Confirm.bHitConfirmed)
	{
		if (AWeapon* Weapon = Character->GetEquippedWeapon())
		{
			float DamageModifier = Confirm.bHeadShot ? Weapon->HeadShotDamageModifier : 1.f;
			UGameplayStatics::ApplyDamage(
				HitCharacter,
				Weapon->GetDamage() * DamageModifier,
				Character->GetController(),
				Weapon,
				UDamageType::StaticClass()
				);
		}
	}
}

void ULagCompensationComponent::ShotgunServerScoreRequest_Implementation(
	const TArray<ABlasterCharacter*>& HitCharacters,
	const FVector_NetQuantize& TraceStart,
	const TArray<FVector_NetQuantize>& HitLocations,
	float HitTime)
{
	FShotgunServerSideRewindResult Confirm = ShotgunServerSideRewind(HitCharacters, TraceStart, HitLocations, HitTime);

	for (auto& HitCharacter : HitCharacters)
	{
		if (HitCharacter == nullptr || Character->GetEquippedWeapon() == nullptr || Character == nullptr) continue;
		float TotalDamage = 0.f;
		if (Confirm.HeadShots.Contains(HitCharacter))
		{
			float HeadShotDamage = Confirm.HeadShots[HitCharacter] * Character->GetEquippedWeapon()->GetDamage() * Character->GetEquippedWeapon()->HeadShotDamageModifier;
			TotalDamage += HeadShotDamage;
		}
		if (Confirm.BodyShots.Contains(HitCharacter))
		{
			float BodyShotDamage = Confirm.BodyShots[HitCharacter] * Character->GetEquippedWeapon()->GetDamage();
			TotalDamage += BodyShotDamage;
		}		
		UGameplayStatics::ApplyDamage(
			HitCharacter,
			TotalDamage,
			Character->GetController(),
			Character->GetEquippedWeapon(),
			UDamageType::StaticClass()
			);
	}
}

void ULagCompensationComponent::ProjectileServerScoreRequest_Implementation(
	ABlasterCharacter* HitCharacter,
	const FVector_NetQuantize& TraceStart,
	const FVector_NetQuantize100& InitialVelocity,
	float HitTime,
	AProjectileBullet* Projectile)
{
	FServerSideRewindResult Confirm = ProjectileServerSideRewind(HitCharacter, TraceStart, InitialVelocity, HitTime);

	if (Character && Character->GetController() && Projectile && HitCharacter && Confirm.bHitConfirmed)
	{
		float DamageModifier = Confirm.bHeadShot ? Projectile->HeadShotDamageModifier : 1.0f;
		UGameplayStatics::ApplyDamage(
			HitCharacter,
			Projectile->Damage * DamageModifier,
			Character->GetController(),
			Projectile,
			UDamageType::StaticClass()
			);
	}
}

void ULagCompensationComponent::RocketServerScoreRequest_Implementation(
	AActor* HitActor,
	const FVector_NetQuantize& TraceStart,
	const FVector_NetQuantize100& InitialVelocity,
	float HitTime,
	AProjectile* Rocket)
{
	// 적중 대상이 캐릭터인 경우 SSR, 주변 반경에 데미지
	if (ABlasterCharacter* HitCharacter = Cast<ABlasterCharacter>(HitActor))
	{
		FRocketServerSideRewindResult Confirm = RocketServerSideRewind(HitCharacter, TraceStart, InitialVelocity, HitTime);
	
		if (Character && Character->GetController() && Character->GetEquippedWeapon() && HitCharacter && Confirm.bHitConfirmed)
		{
			AController* FiringController = Character->GetController();
			if (FiringController)
			{
				UGameplayStatics::ApplyRadialDamageWithFalloff(
					this,							// 월드 객체
					Rocket->Damage,					// 최대 데미지
					10.f,							// 최소 데미지
					Confirm.HitLocation,			// 데미지 시작 지점
					Rocket->DamageInnerRadius,		// 최대 데미지 반경
					Rocket->DamageOuterRadius,		// 최소 데미지 반경
					1.f,							// 데미지 감소 비율
					UDamageType::StaticClass(),		// 데미지 타입 클래스
					TArray<AActor*>(),				// 데미지를 받지 않을 액터
					Rocket,							// 데미지 유발자
					FiringController				// InstigatorController
					);

				Rocket->StartDestroyTimer();
			}
		}
	}
	else // 적중 대상이 캐릭터가 아닌 경우 들어오는 분기
	{
		if (Character)
		{
			AController* FiringController = Character->GetController();
			if (FiringController)
			{
				UGameplayStatics::ApplyRadialDamageWithFalloff(
					this,							// 월드 객체
					Rocket->Damage,					// 최대 데미지
					10.f,							// 최소 데미지
					Rocket->GetActorLocation(),		// 데미지 시작 지점
					Rocket->DamageInnerRadius,		// 최대 데미지 반경
					Rocket->DamageOuterRadius,		// 최소 데미지 반경
					1.f,							// 데미지 감소 비율
					UDamageType::StaticClass(),		// 데미지 타입 클래스
					TArray<AActor*>(),				// 데미지를 받지 않을 액터
					Rocket,							// 데미지 유발자
					FiringController				// InstigatorController
					);
			}
		}
	}
}

FServerSideRewindResult ULagCompensationComponent::ServerSideRewind(
	ABlasterCharacter* HitCharacter,
	const FVector_NetQuantize& TraceStart,
	const FVector_NetQuantize& HitLocation,
	float HitTime)
{
	FFramePackage FrameToCheck = GetFrameToCheck(HitCharacter, HitTime);	// HitTime에 HitCharacter의 히트박스에 대한 위치 정보
	return ConfirmHit(FrameToCheck, TraceStart, HitLocation);	// 위 정보를 기반으로 라인 트레이스를 통해 적중 사실 return
}

FShotgunServerSideRewindResult ULagCompensationComponent::ShotgunServerSideRewind(
	const TArray<ABlasterCharacter*>& HitCharacters,
	const FVector_NetQuantize& TraceStart,
	const TArray<FVector_NetQuantize>& HitLocations,
	float HitTime)
{
	TArray<FFramePackage> FramesToCheck;
	for (ABlasterCharacter* HitCharacter : HitCharacters)
	{
		FramesToCheck.Add(GetFrameToCheck(HitCharacter, HitTime));
	}
	
	return ShotgunConfirmHit(FramesToCheck, TraceStart, HitLocations);
}

FServerSideRewindResult ULagCompensationComponent::ProjectileServerSideRewind(
	ABlasterCharacter* HitCharacter,
	const FVector_NetQuantize& TraceStart,
	const FVector_NetQuantize100& InitialVelocity,
	float HitTime)
{
	FFramePackage FrameToCheck = GetFrameToCheck(HitCharacter, HitTime);	// HitTime에 HitCharacter의 히트박스에 대한 위치 정보
	return ProjectileConfirmHit(FrameToCheck, TraceStart, InitialVelocity);	// 위 정보를 기반으로 라인 트레이스를 통해 적중 사실 return
}

FRocketServerSideRewindResult ULagCompensationComponent::RocketServerSideRewind(
	ABlasterCharacter* HitCharacter,
	const FVector_NetQuantize& TraceStart,
	const FVector_NetQuantize100& InitialVelocity,
	float HitTime)
{
	FFramePackage FrameToCheck = GetFrameToCheck(HitCharacter, HitTime);	// HitTime에 HitCharacter의 히트박스에 대한 위치 정보
	return RocketConfirmHit(FrameToCheck, TraceStart, InitialVelocity);	// 위 정보를 기반으로 라인 트레이스를 통해 적중 사실 return
}

FFramePackage ULagCompensationComponent::GetFrameToCheck(ABlasterCharacter* HitCharacter, float HitTime)
{
	bool bReturn = HitCharacter == nullptr
		|| HitCharacter->GetLagCompensation() == nullptr
		|| HitCharacter->GetLagCompensation()->FrameHistory.GetHead() == nullptr
		|| HitCharacter->GetLagCompensation()->FrameHistory.GetTail() == nullptr;
	if (bReturn) return FFramePackage();
	
	FFramePackage FrameToCheck;
	bool bShouldInterpolate = true;
	
	// 적중당한 캐릭터의 FrameHistory 가져오기
	const TDoubleLinkedList<FFramePackage>& History = HitCharacter->GetLagCompensation()->FrameHistory;
	const float OldestHistoryTime = History.GetTail()->GetValue().Time;
	const float NewestHistoryTime = History.GetHead()->GetValue().Time;
	
	if (OldestHistoryTime > HitTime)
	{
		// 발사자의 핑이 너무 높은 경우 들어오는 분기, 정상적인 플레이가 불가능
		return FFramePackage();
	}
	if (OldestHistoryTime == HitTime)
	{
		// 이럴 일은 거의 없지만, 가장 마지막 프레임
		// 즉, 저장되어 지워지기 직전인 위치 정보의 시간과 HitTime이 일치하는 경우 들어오는 분기
		// 이것도 핑이 상당히 높아 정상적인 플레이가 불가능한 경우가 대다수
		FrameToCheck = History.GetTail()->GetValue();
		bShouldInterpolate = false;
	}
	if (NewestHistoryTime <= HitTime)
	{
		// 핑이 0에 육박하는 경우 들어오는 분기, 보통은 리슨 서버가 여기로 들어옴
		FrameToCheck = History.GetHead()->GetValue();
		bShouldInterpolate = false;
	}

	TDoubleLinkedList<FFramePackage>::TDoubleLinkedListNode* Younger = History.GetHead();
	TDoubleLinkedList<FFramePackage>::TDoubleLinkedListNode* Older = Younger;
	while (Older != nullptr)
	{
		// OlderTime < HitTime < YoungerTime 이 될 때까지 반복
		if (Older->GetValue().Time <= HitTime)
		{
			break;
		}
		if (Older->GetNextNode() != nullptr)
		{
			Younger = Older;				// Younger는 현재 Older로 이동
			Older = Older->GetNextNode();	// Older는 다음 노드로 이동
		}
		else
		{
			// 마지막 노드 도달 시 탐색 종료
			break;
		}
	}
	if (Older->GetValue().Time == HitTime)
	{
		// HitTime과 Older의 Time이 일치할 가능성은 거의 없지만, 혹시 일치하면 바로 할당
		FrameToCheck = Older->GetValue();
		bShouldInterpolate = false;
	}
	if (bShouldInterpolate)
	{
		// 서버가 저장해둔 Time과 HitTime이 정확히 일치할 일은 거의 없으므로 보통 Interpolate해야 함
		FrameToCheck = InterpBetweenFrames(Older->GetValue(), Younger->GetValue(), HitTime);
	}
	FrameToCheck.Character = HitCharacter;
	return FrameToCheck;
}

FFramePackage ULagCompensationComponent::InterpBetweenFrames(
	const FFramePackage& OlderFrame,
	const FFramePackage& YoungerFrame,
	float HitTime)
{
	// Older와 Younger사이에 존재하는 HitTime의 비율이 어느정도인지 구함, 얼마나 Interpolate해야 하는지
	const float Distance = YoungerFrame.Time - OlderFrame.Time;
	const float InterpFraction = FMath::Clamp((HitTime - OlderFrame.Time) / Distance, 0.f, 1.f);

	// 빈 FramePackage 하나 선언
	FFramePackage InterpFramePackage;
	InterpFramePackage.Time = HitTime;

	// 모든 BoxInformation을 순회하며 Older와 Younger의 사이 HitTime에 위치해있을 FramePackage를 구한다
	for (auto& YoungerPair : YoungerFrame.HitBoxInfo)
	{
		const FName& BoxInfoName = YoungerPair.Key;

		const FBoxInformation& OlderBox = OlderFrame.HitBoxInfo[BoxInfoName];
		const FBoxInformation& YoungerBox = YoungerPair.Value;

		FBoxInformation InterpBoxInfo;
		InterpBoxInfo.Location = FMath::VInterpTo(OlderBox.Location, YoungerBox.Location, 1.f, InterpFraction);
		InterpBoxInfo.Rotation = FMath::RInterpTo(OlderBox.Rotation, YoungerBox.Rotation, 1.f, InterpFraction);
		InterpBoxInfo.BoxExtent = YoungerBox.BoxExtent;

		InterpFramePackage.HitBoxInfo.Add(BoxInfoName, InterpBoxInfo);
	}

	return InterpFramePackage;
}

FServerSideRewindResult ULagCompensationComponent::ConfirmHit(
	const FFramePackage& Package,			// HitTime에 캐릭터의 히트박스가 어디에 있었는지에 대한 정보를 가진 FramePackage
	const FVector_NetQuantize& TraceStart,	// LineTrace 시작 지점
	const FVector_NetQuantize& HitLocation)	// LineTrace 끝 지점(조금 더 나아가야 함)
{
	if (Package.Character == nullptr) return FServerSideRewindResult();

	// 현재 캐릭터의 BoxComponent 위치를 캐싱
	FFramePackage CurrentFrame;
	CacheBoxPositions(Package.Character, CurrentFrame);

	// 캐릭터의 BoxComponent 위치를 Interpolate된 FramePackage 위치로 옮겨줌
	MoveBoxes(Package.Character, Package);
	// HitBox로만 판정 볼 거기 때문에 Mesh는 콜리전 꺼줌
	EnableCharacterMeshCollision(Package.Character, ECollisionEnabled::NoCollision);
	// 머리부터 히트박스 켤 거기 때문에 일단 전부 꺼줌
	OffCollisionHitBoxes(Package.Character);

	// 머리 히트박스 활성화
	UBoxComponent* HeadBox = Package.Character->HitCollisionBoxes[FName("head")];
	HeadBox->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
	HeadBox->SetCollisionResponseToChannel(ECC_HitBox, ECR_Block);

	// HitLocation은 메쉬에서 끝나버렸기 때문에 조금 더 나아간 위치까지 LineTrace
	const FVector TraceEnd = TraceStart + (HitLocation - TraceStart) * 1.25f;
	if (UWorld* World = GetWorld())
	{
		FHitResult ConfirmHitResult;
		World->LineTraceSingleByChannel(
			ConfirmHitResult,
			TraceStart,
			TraceEnd,
			ECC_HitBox
			);
		if (ConfirmHitResult.bBlockingHit) // 헤드샷인 경우 들어오는 분기
		{
			ResetHitBoxes(Package.Character, CurrentFrame);
			EnableCharacterMeshCollision(Package.Character, ECollisionEnabled::QueryAndPhysics);
			return FServerSideRewindResult{ true, true };
		}
		else // 헤드샷이 아닌 경우 들어오는 분기, 나머지 히트박스들도 체크
		{
			for (auto& HitBoxPair : Package.Character->HitCollisionBoxes)
			{
				// 나머지 히트박스 활성화
				if (HitBoxPair.Value != nullptr)
				{
					HitBoxPair.Value->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
					HitBoxPair.Value->SetCollisionResponseToChannel(ECC_HitBox, ECR_Block);
				}
			}
			World->LineTraceSingleByChannel(
				ConfirmHitResult,
				TraceStart,
				TraceEnd,
				ECC_HitBox
				);
			if (ConfirmHitResult.bBlockingHit)
			{
				// 적중 결과 있는 경우 바디샷 판정으로 return
				
				ResetHitBoxes(Package.Character, CurrentFrame);
				EnableCharacterMeshCollision(Package.Character, ECollisionEnabled::QueryAndPhysics);
				return FServerSideRewindResult{ true, false };
			}
		}
	}

	// 아무것도 적중하지 못 한 경우
	ResetHitBoxes(Package.Character, CurrentFrame);
	EnableCharacterMeshCollision(Package.Character, ECollisionEnabled::QueryAndPhysics);
	return FServerSideRewindResult{ false, false };
}

FShotgunServerSideRewindResult ULagCompensationComponent::ShotgunConfirmHit(
	const TArray<FFramePackage>& FramePackages,
	const FVector_NetQuantize& TraceStart,
	const TArray<FVector_NetQuantize>& HitLocations)
{
	for (auto& Frame : FramePackages)
	{
		if (Frame.Character == nullptr) return FShotgunServerSideRewindResult();
	}
	
	FShotgunServerSideRewindResult ShotgunResult;
	TArray<FFramePackage> CurrentFrames;
	TArray<FVector_NetQuantize> HeadShotPellets;
	for (auto& Frame : FramePackages)
	{
		// 캐릭터의 BoxComponent 위치를 FramePackage 위치로 옮겨줌
		FFramePackage CurrentFrame;
		CacheBoxPositions(Frame.Character, CurrentFrame);
		MoveBoxes(Frame.Character, Frame);
		EnableCharacterMeshCollision(Frame.Character, ECollisionEnabled::NoCollision);
		// 머리부터 히트박스 켤 거기 때문에 일단 전부 꺼줌
		OffCollisionHitBoxes(Frame.Character);
		CurrentFrames.Add(CurrentFrame);
	}
	for (auto& Frame : FramePackages)
	{
		// 머리 히트박스 활성화
		UBoxComponent* HeadBox = Frame.Character->HitCollisionBoxes[FName("head")];
		HeadBox->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
		HeadBox->SetCollisionResponseToChannel(ECC_HitBox, ECR_Block);
	}
	// 머리 히트박스만 활성화한 상태로 라인 트레이스 검증 시작
	UWorld* World = GetWorld();
	for (auto& HitLocation : HitLocations)
	{
		const FVector TraceEnd = TraceStart + (HitLocation - TraceStart) * 1.25f;
		if (World)
		{
			FHitResult ConfirmHitResult;
			World->LineTraceSingleByChannel(
				ConfirmHitResult,
				TraceStart,
				TraceEnd,
				ECC_HitBox
				);
			// 적중 결과에 따라 TMap에 값 추가
			ABlasterCharacter* BlasterCharacter = Cast<ABlasterCharacter>(ConfirmHitResult.GetActor());
			if (BlasterCharacter)
			{				
				if (ShotgunResult.HeadShots.Contains(BlasterCharacter))
				{
					ShotgunResult.HeadShots[BlasterCharacter]++;
				}
				else
				{
					ShotgunResult.HeadShots.Emplace(BlasterCharacter, 1);
				}
				// 검증 결과 적중한 펠릿이므로, 바디샷에서 또 다시 적중 결과를 뱉지 않도록 기록
				HeadShotPellets.Add(HitLocation);
			}
		}
	}
	for (auto& Frame : FramePackages)
	{
		// 머리를 제외한 나머지 히트박스도 활성화
		for (auto& HitBoxPair : Frame.Character->HitCollisionBoxes)
		{
			if (HitBoxPair.Value != nullptr)
			{
				HitBoxPair.Value->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
				HitBoxPair.Value->SetCollisionResponseToChannel(ECC_HitBox, ECR_Block);
			}
		}
		// 머리 히트박스는 비활성화
		UBoxComponent* HeadBox = Frame.Character->HitCollisionBoxes[FName("head")];
		HeadBox->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	}
	// 바디샷 라인 트레이스 검증 시작
	for (auto& HitLocation : HitLocations)
	{
		const FVector TraceEnd = TraceStart + (HitLocation - TraceStart) * 1.25f;
		if (World)
		{
			FHitResult ConfirmHitResult;
			World->LineTraceSingleByChannel(
				ConfirmHitResult,
				TraceStart,
				TraceEnd,
				ECC_HitBox
				);
			// 적중 시 이미 HeadShot 판정을 받은 펠릿인지도 체크
			ABlasterCharacter* BlasterCharacter = Cast<ABlasterCharacter>(ConfirmHitResult.GetActor());
			if (BlasterCharacter && !HeadShotPellets.Contains(HitLocation))
			{				
				if (ShotgunResult.BodyShots.Contains(BlasterCharacter))
				{
					ShotgunResult.BodyShots[BlasterCharacter]++;
				}
				else
				{
					ShotgunResult.BodyShots.Emplace(BlasterCharacter, 1);
				}
			}
		}
	}
	for (auto& Frame : CurrentFrames)
	{
		ResetHitBoxes(Frame.Character, Frame);
		EnableCharacterMeshCollision(Frame.Character, ECollisionEnabled::QueryAndPhysics);
	}
	
	return ShotgunResult;
}

FServerSideRewindResult ULagCompensationComponent::ProjectileConfirmHit(
	const FFramePackage& Package,
	const FVector_NetQuantize& TraceStart,
	const FVector_NetQuantize100& InitialVelocity)
{
	if (Package.Character == nullptr) return FServerSideRewindResult();

	// 현재 캐릭터의 BoxComponent 위치를 캐싱
	FFramePackage CurrentFrame;
	CacheBoxPositions(Package.Character, CurrentFrame);

	// 캐릭터의 BoxComponent 위치를 Interpolate된 FramePackage 위치로 옮겨줌
	MoveBoxes(Package.Character, Package);
	// HitBox로만 판정 볼 거기 때문에 Mesh는 콜리전 꺼줌
	EnableCharacterMeshCollision(Package.Character, ECollisionEnabled::NoCollision);
	// 머리부터 히트박스 켤 거기 때문에 일단 전부 꺼줌
	OffCollisionHitBoxes(Package.Character);

	// 머리 히트박스 활성화
	UBoxComponent* HeadBox = Package.Character->HitCollisionBoxes[FName("head")];
	HeadBox->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
	HeadBox->SetCollisionResponseToChannel(ECC_HitBox, ECR_Block);

	// 투사체 경로 예측 세팅
	FPredictProjectilePathParams PathParams;
	PathParams.bTraceWithCollision = true;
	PathParams.MaxSimTime = MaxRecordTime;
	PathParams.LaunchVelocity = InitialVelocity;
	PathParams.StartLocation = TraceStart;
	PathParams.SimFrequency = 15.f;
	PathParams.ProjectileRadius = 5.f;
	PathParams.TraceChannel = ECC_HitBox;
	PathParams.ActorsToIgnore.Add(GetOwner());

	// 투사체 경로 예측 함수 호출
	FPredictProjectilePathResult PathResult;
	UGameplayStatics::PredictProjectilePath(this, PathParams, PathResult);

	if (PathResult.HitResult.bBlockingHit) // 헤드샷인 경우 들어오는 분기
	{
		if (PathResult.HitResult.Component.IsValid())
		{
			UBoxComponent* Box = Cast<UBoxComponent>(PathResult.HitResult.Component);
		}

		ResetHitBoxes(Package.Character, CurrentFrame);
		EnableCharacterMeshCollision(Package.Character, ECollisionEnabled::QueryAndPhysics);
		return FServerSideRewindResult{ true, true };
	}
	else // 헤드샷이 아닌 경우 들어우는 분기
	{
		for (auto& HitBoxPair : Package.Character->HitCollisionBoxes)
		{
			// 나머지 히트박스 활성화
			if (HitBoxPair.Value != nullptr)
			{
				HitBoxPair.Value->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
				HitBoxPair.Value->SetCollisionResponseToChannel(ECC_HitBox, ECR_Block);
			}
		}
		// 투사체 경로 예측 함수 호출
		UGameplayStatics::PredictProjectilePath(this, PathParams, PathResult);
		
		if (PathResult.HitResult.bBlockingHit) // 바디샷인 경우 들어오는 분기
		{
			ResetHitBoxes(Package.Character, CurrentFrame);
			EnableCharacterMeshCollision(Package.Character, ECollisionEnabled::QueryAndPhysics);
			return FServerSideRewindResult{ true, false };
		}
	}

	// 아무것도 적중하지 못 한 경우
	ResetHitBoxes(Package.Character, CurrentFrame);
	EnableCharacterMeshCollision(Package.Character, ECollisionEnabled::QueryAndPhysics);
	return FServerSideRewindResult();
}

FRocketServerSideRewindResult ULagCompensationComponent::RocketConfirmHit(
	const FFramePackage& Package,
	const FVector_NetQuantize& TraceStart,
	const FVector_NetQuantize100& InitialVelocity)
{
	if (Package.Character == nullptr) return FRocketServerSideRewindResult();

	// 현재 캐릭터의 BoxComponent 위치를 캐싱
	FFramePackage CurrentFrame;
	CacheBoxPositions(Package.Character, CurrentFrame);

	// 캐릭터의 BoxComponent 위치를 Interpolate된 FramePackage 위치로 옮겨줌
	MoveBoxes(Package.Character, Package);
	// HitBox로만 판정 볼 거기 때문에 Mesh는 콜리전 꺼줌
	EnableCharacterMeshCollision(Package.Character, ECollisionEnabled::NoCollision);
	// 머리부터 히트박스 켤 거기 때문에 일단 전부 꺼줌
	OffCollisionHitBoxes(Package.Character);

	// 머리 히트박스 활성화
	UBoxComponent* HeadBox = Package.Character->HitCollisionBoxes[FName("head")];
	HeadBox->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
	HeadBox->SetCollisionResponseToChannel(ECC_HitBox, ECR_Block);

	// 투사체 경로 예측 세팅
	FPredictProjectilePathParams PathParams;
	PathParams.bTraceWithCollision = true;
	PathParams.MaxSimTime = MaxRecordTime;
	PathParams.LaunchVelocity = InitialVelocity;
	PathParams.StartLocation = TraceStart;
	PathParams.SimFrequency = 15.f;
	PathParams.ProjectileRadius = 5.f;
	PathParams.TraceChannel = ECC_HitBox;
	PathParams.ActorsToIgnore.Add(GetOwner());

	// 투사체 경로 예측 함수 호출
	FPredictProjectilePathResult PathResult;
	UGameplayStatics::PredictProjectilePath(this, PathParams, PathResult);

	if (PathResult.HitResult.bBlockingHit) // 헤드샷인 경우 들어오는 분기
	{
		if (PathResult.HitResult.Component.IsValid())
		{
			UBoxComponent* Box = Cast<UBoxComponent>(PathResult.HitResult.Component);
		}

		ResetHitBoxes(Package.Character, CurrentFrame);
		EnableCharacterMeshCollision(Package.Character, ECollisionEnabled::QueryAndPhysics);
		return FRocketServerSideRewindResult{ true, true, PathResult.HitResult.Location };
	}
	else // 헤드샷이 아닌 경우 들어우는 분기
	{
		for (auto& HitBoxPair : Package.Character->HitCollisionBoxes)
		{
			// 나머지 히트박스 활성화
			if (HitBoxPair.Value != nullptr)
			{
				HitBoxPair.Value->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
				HitBoxPair.Value->SetCollisionResponseToChannel(ECC_HitBox, ECR_Block);
			}
		}
		// 투사체 경로 예측 함수 호출
		UGameplayStatics::PredictProjectilePath(this, PathParams, PathResult);
		
		if (PathResult.HitResult.bBlockingHit) // 바디샷인 경우 들어오는 분기
		{
			ResetHitBoxes(Package.Character, CurrentFrame);
			EnableCharacterMeshCollision(Package.Character, ECollisionEnabled::QueryAndPhysics);
			return FRocketServerSideRewindResult{ true, false, PathResult.HitResult.Location };
		}
	}

	// 아무것도 적중하지 못 한 경우
	ResetHitBoxes(Package.Character, CurrentFrame);
	EnableCharacterMeshCollision(Package.Character, ECollisionEnabled::QueryAndPhysics);
	return FRocketServerSideRewindResult();
}

void ULagCompensationComponent::CacheBoxPositions(ABlasterCharacter* HitCharacter, FFramePackage& OutFramePackage)
{
	if (HitCharacter == nullptr) return;

	OutFramePackage.Character = HitCharacter;
	for (auto& HitBoxPair : HitCharacter->HitCollisionBoxes)
	{
		if (HitBoxPair.Value != nullptr)
		{
			FBoxInformation BoxInfo;
			BoxInfo.Location = HitBoxPair.Value->GetComponentLocation();
			BoxInfo.Rotation = HitBoxPair.Value->GetComponentRotation();
			BoxInfo.BoxExtent = HitBoxPair.Value->GetScaledBoxExtent();
			OutFramePackage.HitBoxInfo.Add(HitBoxPair.Key, BoxInfo);
		}
	}
}

void ULagCompensationComponent::MoveBoxes(ABlasterCharacter* HitCharacter, const FFramePackage& Package)
{
	if (HitCharacter == nullptr) return;

	for (auto& HitBoxPair : HitCharacter->HitCollisionBoxes)
	{
		if (HitBoxPair.Value != nullptr)
		{
			HitBoxPair.Value->SetWorldLocation(Package.HitBoxInfo[HitBoxPair.Key].Location);
			HitBoxPair.Value->SetWorldRotation(Package.HitBoxInfo[HitBoxPair.Key].Rotation);
			HitBoxPair.Value->SetBoxExtent(Package.HitBoxInfo[HitBoxPair.Key].BoxExtent);
		}
	}
}

void ULagCompensationComponent::ResetHitBoxes(ABlasterCharacter* HitCharacter, const FFramePackage& Package)
{
	if (HitCharacter == nullptr) return;
	
	for (auto& HitBoxPair : HitCharacter->HitCollisionBoxes)
	{
		if (HitBoxPair.Value != nullptr)
		{
			HitBoxPair.Value->SetWorldLocation(Package.HitBoxInfo[HitBoxPair.Key].Location);
			HitBoxPair.Value->SetWorldRotation(Package.HitBoxInfo[HitBoxPair.Key].Rotation);
			HitBoxPair.Value->SetBoxExtent(Package.HitBoxInfo[HitBoxPair.Key].BoxExtent);
			HitBoxPair.Value->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
		}
	}
}

void ULagCompensationComponent::EnableCharacterMeshCollision(ABlasterCharacter* HitCharacter, ECollisionEnabled::Type CollisionEnabled)
{
	if (HitCharacter && HitCharacter->GetMesh())
	{
		HitCharacter->GetMesh()->SetCollisionEnabled(CollisionEnabled);
	}
}

void ULagCompensationComponent::OffCollisionHitBoxes(ABlasterCharacter* HitCharacter)
{
	if (HitCharacter == nullptr) return;
	
	for (auto& HitBoxPair : HitCharacter->HitCollisionBoxes)
	{
		if (HitBoxPair.Value != nullptr)
		{
			HitBoxPair.Value->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		}
	}
}

void ULagCompensationComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);
	
	SaveFramePackage();
}

void ULagCompensationComponent::SaveFramePackage()
{
	if (Character == nullptr || !Character->HasAuthority()) return;
	// FrameHistory가 비어있으면 현재 HitBox 정보 바로 저장
	if (FrameHistory.Num() <= 1)
	{
		FFramePackage ThisFrame;
		SaveFramePackage(ThisFrame);
		FrameHistory.AddHead(ThisFrame);
	}
	else
	{
		// MaxRecordTime보다 길게 저장된 HitBox 정보는 삭제
		float HistoryLength = FrameHistory.GetHead()->GetValue().Time - FrameHistory.GetTail()->GetValue().Time;
		while (HistoryLength > MaxRecordTime)
		{
			FrameHistory.RemoveNode(FrameHistory.GetTail());
			HistoryLength = FrameHistory.GetHead()->GetValue().Time - FrameHistory.GetTail()->GetValue().Time;
		}

		// 새로운 HitBox 위치 정보 저장
		FFramePackage ThisFrame;
		SaveFramePackage(ThisFrame);
		FrameHistory.AddHead(ThisFrame);

		//ShowFramePackage(ThisFrame, FColor::Red);
	}
}

void ULagCompensationComponent::SaveFramePackage(FFramePackage& Package)
{
	Character = Character == nullptr ? Cast<ABlasterCharacter>(GetOwner()) : Character;
	if (Character)
	{
		// 시간이 오래될 수록 수치가 낮다는 것을 명심
		Package.Time = GetWorld()->GetTimeSeconds();
		Package.Character = Character;
		for (auto& BoxPair : Character->HitCollisionBoxes)
		{
			FBoxInformation BoxInformation;
			BoxInformation.Location = BoxPair.Value->GetComponentLocation();
			BoxInformation.Rotation = BoxPair.Value->GetComponentRotation();
			BoxInformation.BoxExtent = BoxPair.Value->GetScaledBoxExtent();
			Package.HitBoxInfo.Add(BoxPair.Key, BoxInformation);
		}
	}
}

void ULagCompensationComponent::ShowFramePackage(const FFramePackage& Package, const FColor& Color)
{
	for (auto& BoxInfo : Package.HitBoxInfo)
	{
		DrawDebugBox(
			GetWorld(),
			BoxInfo.Value.Location,
			BoxInfo.Value.BoxExtent,
			FQuat(BoxInfo.Value.Rotation),
			Color,
			false,
			4.f);
	}
}