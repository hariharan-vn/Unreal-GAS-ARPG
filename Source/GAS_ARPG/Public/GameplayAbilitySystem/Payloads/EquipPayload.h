// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "UObject/Object.h"
#include "EquipPayload.generated.h"

USTRUCT(BlueprintType)
struct FWeaponAbilityConfig
{
	GENERATED_BODY()


};

UCLASS()
class GAS_ARPG_API UEquipPayload : public UObject
{
	GENERATED_BODY()

	UPROPERTY(EditDefaultsOnly)
	FGameplayTag AbilityTag;

	UPROPERTY(EditDefaultsOnly)
	TSubclassOf<UAnimInstance> AnimBPClass;
};
