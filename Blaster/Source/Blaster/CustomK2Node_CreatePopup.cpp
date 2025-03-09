#include "CustomK2Node_CreatePopup.h"

#include "BlueprintActionDatabaseRegistrar.h"
#include "BlueprintNodeSpawner.h"
#include "EdGraphSchema_K2.h"
#include "Blueprint/UserWidget.h"
#include "UObject/Class.h"
#include "UObject/UnrealType.h"
#include "EdGraph/EdGraphSchema.h"

void UCustomK2Node_CreatePopup::GetMenuActions(FBlueprintActionDatabaseRegistrar& ActionRegistrar) const
{
	UClass* ActionKey = GetClass();
	if (ActionRegistrar.IsOpenForRegistration(ActionKey))
	{
		UBlueprintNodeSpawner* NodeSpawner = UBlueprintNodeSpawner::Create(GetClass());
		ActionRegistrar.AddBlueprintAction(ActionKey, NodeSpawner);
	}
}

FText UCustomK2Node_CreatePopup::GetMenuCategory() const
{
	return FText::FromString(TEXT("Custom Nodes"));
}

FText UCustomK2Node_CreatePopup::GetNodeTitle(ENodeTitleType::Type TitleType) const
{
	return FText::FromString(TEXT("Create Popup"));
}

FText UCustomK2Node_CreatePopup::GetTooltipText() const
{
	return FText::FromString(TEXT("Spawns an object of the specified class and exposes its ExposeOnSpawn properties as inputs."));
}

void UCustomK2Node_CreatePopup::AllocateDefaultPins()
{
	// 입력 실행 핀 생성
	CreatePin(EGPD_Input, UEdGraphSchema_K2::PC_Exec, TEXT(""));

	// 출력 실행 핀 생성
	CreatePin(EGPD_Output, UEdGraphSchema_K2::PC_Exec, TEXT(""));
	
	// 고정 핀 추가
	UEdGraphPin* SpawnClassPin = CreatePin(EGPD_Input, UEdGraphSchema_K2::PC_Class, UUserWidget::StaticClass(), TEXT("Spawn Popup Class"));
	SpawnClassPin->PinFriendlyName = FText::FromString(TEXT("Spawn Popup Class"));
	
	// (여기서 다른 고정 입력 핀들을 추가할 수도 있음)

	// 결과 오브젝트를 반환하는 출력 핀
	UEdGraphPin* ReturnPin = CreatePin(EGPD_Output, UEdGraphSchema_K2::PC_Object, UUserWidget::StaticClass(), UEdGraphSchema_K2::PN_ReturnValue);
	ReturnPin->PinFriendlyName = FText::FromString(TEXT("Spawned Popup"));
}

void UCustomK2Node_CreatePopup::PinDefaultValueChanged(UEdGraphPin* Pin)
{
	Super::PinDefaultValueChanged(Pin);
	// 만약 SpawnClass가 유효하다면, 해당 클래스의 ExposeOnSpawn 프로퍼티들을 입력 핀으로 동적으로 추가합니다.
	CreateExposeOnSpawnPins();
}

void UCustomK2Node_CreatePopup::ExpandNode(class FKismetCompilerContext& CompilerContext, UEdGraph* SourceGraph)
{
	Super::ExpandNode(CompilerContext, SourceGraph);
}

void UCustomK2Node_CreatePopup::CreateExposeOnSpawnPins()
{	
	// SpawnClass 핀에서 값을 읽어옵니다.
	UEdGraphPin* SpawnClassPin = FindPin(TEXT("Spawn Popup Class"));
	if (!SpawnClassPin)
	{
		return;
	}

	// SpawnClass 핀의 기본값이 지정된 경우 해당 클래스를 가져옵니다.
	UClass* ClassToSpawn = nullptr;
	if (SpawnClassPin->DefaultObject != nullptr)
	{
		ClassToSpawn = Cast<UClass>(SpawnClassPin->DefaultObject);
	}

	if (!ClassToSpawn)
	{
		// SpawnClass가 아직 지정되지 않았다면 스폰 시 노출 핀을 만들지 않습니다.
		return;
	}

	// 해당 클래스의 모든 UPROPERTY를 순회하며 meta 데이터를 확인합니다.
	for (TFieldIterator<FProperty> It(ClassToSpawn); It; ++It)
	{
		FProperty* Property = *It;
		FString PinName = Property->GetName();
		if (Property->HasMetaData(TEXT("ExposeOnSpawn")))
		{
			FEdGraphPinType PinType;
			if(GetDefault<UEdGraphSchema_K2>()->ConvertPropertyToPinType(Property, PinType))
			{
				UEdGraphPin* NewPin = CreatePin(EGPD_Input, PinType.PinCategory, TEXT(""), nullptr, *PinName);
				NewPin->PinFriendlyName = FText::FromString(PinName);
			}
			else
			{
				// 변환 실패 시 기본값 처리 (예: 문자열 타입)
				UEdGraphPin* NewPin = CreatePin(EGPD_Input, UEdGraphSchema_K2::PC_String, TEXT(""), nullptr, *PinName);
				NewPin->PinFriendlyName = FText::FromString(PinName);
			}
		}
	}
}
