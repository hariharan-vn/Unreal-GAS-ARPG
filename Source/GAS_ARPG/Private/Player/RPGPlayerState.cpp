// Fill out your copyright notice in the Description page of Project Settings.


#include "Player/RPGPlayerState.h"

#include "Character/GAS_ARPGCharacter.h"
#include "GameplayAbilitySystem/AttributeSets/BasicAttributeSet.h"


ARPGPlayerState::ARPGPlayerState()
{
	AbilitySystemComponent = CreateDefaultSubobject<UAbilitySystemComponent>(TEXT("AbilitySystemComponent"));
	AbilitySystemComponent->SetReplicationMode(EGameplayEffectReplicationMode::Mixed);
	AbilitySystemComponent->SetIsReplicated(true);

	BasicAttributeSet = CreateDefaultSubobject<UBasicAttributeSet>(TEXT("AttributeSet"));

	SetNetUpdateFrequency(100.f);
}

void ARPGPlayerState::PushASCToPawn()
{
	AGAS_ARPGCharacter* ARPGCharacter = Cast<AGAS_ARPGCharacter>(GetPawn());
	if (!ARPGCharacter) return;

	ARPGCharacter->InitializePawnASC(this);
}
