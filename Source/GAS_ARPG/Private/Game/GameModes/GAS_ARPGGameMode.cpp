// Copyright Epic Games, Inc. All Rights Reserved.

#include "Game/GameModes/GAS_ARPGGameMode.h"
#include "Player/GAS_ARPGPlayerController.h"
#include "Character/GAS_ARPGCharacter.h"
#include "UObject/ConstructorHelpers.h"

AGAS_ARPGGameMode::AGAS_ARPGGameMode()
{
	// use our custom PlayerController class
	PlayerControllerClass = AGAS_ARPGPlayerController::StaticClass();

	// set default pawn class to our Blueprinted character
	static ConstructorHelpers::FClassFinder<APawn> PlayerPawnBPClass(TEXT("/Game/TopDown/Blueprints/BP_TopDownCharacter"));
	if (PlayerPawnBPClass.Class != nullptr)
	{
		DefaultPawnClass = PlayerPawnBPClass.Class;
	}

	// set default controller to our Blueprinted controller
	static ConstructorHelpers::FClassFinder<APlayerController> PlayerControllerBPClass(TEXT("/Game/TopDown/Blueprints/BP_TopDownPlayerController"));
	if(PlayerControllerBPClass.Class != NULL)
	{
		PlayerControllerClass = PlayerControllerBPClass.Class;
	}
}