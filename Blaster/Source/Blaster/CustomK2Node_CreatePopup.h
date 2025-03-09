#pragma once

#include "CoreMinimal.h"
#include "K2Node.h"
#include "CustomK2Node_CreatePopup.generated.h"

UCLASS()
class BLASTER_API UCustomK2Node_CreatePopup : public UK2Node
{
	GENERATED_BODY()

	virtual void GetMenuActions(FBlueprintActionDatabaseRegistrar& ActionRegistrar) const override;
	virtual FText GetMenuCategory() const override;
	virtual FText GetNodeTitle(ENodeTitleType::Type TitleType) const override;
	virtual FText GetTooltipText() const override;
	virtual void AllocateDefaultPins() override;
	virtual void PinDefaultValueChanged(UEdGraphPin* Pin) override;
	virtual void ExpandNode(class FKismetCompilerContext& CompilerContext, UEdGraph* SourceGraph) override;

	UPROPERTY(EditAnywhere, Category = "CreatePopup")
	TSubclassOf<UUserWidget> PopupClass;

	void CreateExposeOnSpawnPins();
};
