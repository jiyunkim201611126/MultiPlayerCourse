#pragma once

#include "CoreMinimal.h"
#include "TeamsGameMode.h"
#include "CaptureTheFlagGameMode.generated.h"

UCLASS()
class BLASTER_API ACaptureTheFlagGameMode : public ATeamsGameMode
{
	GENERATED_BODY()

public:
	// 팀별 킬 점수가 필요하지 않으므로 override해 사용
	virtual void PlayerEliminated(ABlasterCharacter* EliminatedCharacter, ABlasterPlayerController* VictimController, ABlasterPlayerController* AttackerController) override;

	// Flag가 FlagZone에 도달했을 경우 호출되는 함수
	bool FlagCaptured(const class AFlag* Flag, const class AFlagZone* Zone) const;
};
