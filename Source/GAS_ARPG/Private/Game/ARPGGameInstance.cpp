// Fill out your copyright notice in the Description page of Project Settings.


#include "Game/ARPGGameInstance.h"
#include "AbilitySystemGlobals.h"


void UARPGGameInstance::Init()
{
	Super::Init();
	
	UAbilitySystemGlobals::Get().InitGlobalData();
}
