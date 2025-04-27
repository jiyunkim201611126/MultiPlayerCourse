#include "CaptureTheFlagGameMode.h"
#include "Blaster/Weapon/Flag.h"
#include "Blaster/CaptureTheFlag/FlagZone.h"
#include "Blaster/GameState/BlasterGameState.h"

void ACaptureTheFlagGameMode::PlayerEliminated(
	ABlasterCharacter* EliminatedCharacter,
	ABlasterPlayerController* VictimController,
	ABlasterPlayerController* AttackerController)
{
	// 팀별 킬 점수가 필요하지 않으므로 TeamsGameMode의 PlayerEliminated를 호출하지 않음
	ABlasterGameMode::PlayerEliminated(EliminatedCharacter, VictimController, AttackerController);
}

bool ACaptureTheFlagGameMode::FlagCaptured(const AFlag* Flag, const AFlagZone* Zone) const
{
	bool bValidCapture = Flag->GetTeam() != Zone->Team;
	ABlasterGameState* BlasterGameState = Cast<ABlasterGameState>(GameState);
	if (BlasterGameState && bValidCapture)
	{
		if (Zone->Team == ETeam::ET_BlueTeam)
		{
			BlasterGameState->BlueTeamScores();
			return true;
		}
		if (Zone->Team == ETeam::ET_RedTeam)
		{
			BlasterGameState->RedTeamScores();
			return true;
		}
	}
	return false;
}
