#include "BlasterGameMode.h"
#include "Blaster/Character/BlasterCharacter.h"
#include "Blaster/PlayerController/BlasterPlayerController.h"

void ABlasterGameMode::PlayerEliminated(ABlasterCharacter* EliminatedCharacter,
	ABlasterPlayerController* VictimController,
	ABlasterPlayerController* AttackerController)
{
	if (EliminatedCharacter)
	{
		EliminatedCharacter->Elim();
	}
}
