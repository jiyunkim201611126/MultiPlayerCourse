#include "Flag.h"
#include "Components/StaticMeshComponent.h"
#include "Components/SphereComponent.h"
#include "Components/WidgetComponent.h"

AFlag::AFlag()
{
	PrimaryActorTick.bCanEverTick = true;

	FlagMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("FlagMesh"));
	FlagMesh->SetCollisionResponseToAllChannels(ECR_Ignore);
	FlagMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	SetRootComponent(FlagMesh);
	GetAreaSphere()->SetupAttachment(FlagMesh);
	GetPickupWidget()->SetupAttachment(FlagMesh);
}

void AFlag::OnConstruction(const FTransform& Transform)
{
	Super::Super::OnConstruction(Transform);
	
	FlagMesh->SetRenderCustomDepth(true);
	FlagMesh->SetCustomDepthStencilValue(static_cast<uint8>(WeaponGrade));
	FlagMesh->MarkRenderStateDirty();
	EnableCustomDepth(true);
}

