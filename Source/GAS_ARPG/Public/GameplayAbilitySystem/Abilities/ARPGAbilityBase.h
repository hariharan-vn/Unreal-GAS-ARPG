// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Abilities/GameplayAbility.h"
#include "ARPGAbilityBase.generated.h"

/**
 * 
 */
UCLASS()
class GAS_ARPG_API UARPGAbilityBase : public UGameplayAbility
{
	GENERATED_BODY()

public:
	UARPGAbilityBase();

	virtual void PostInitProperties() override;

protected:
	// ── Called in each child constructor ──────
	// Reads configured tags and applies them
	void InitializeAbility();

	// ── Configured in Blueprint per ability ───

	// What this ability IS
	UPROPERTY(EditDefaultsOnly, Category = "Ability|Tags")
	FGameplayTag AbilityIdentityTag;

	// What event activates this ability
	UPROPERTY(EditDefaultsOnly, Category = "Ability|Tags")
	FGameplayTag ActivationEventTag;

	// Tags that block this ability
	UPROPERTY(EditDefaultsOnly, Category = "Ability|Tags")
	FGameplayTagContainer BlockedByTags;

private:
	bool bAbilityInitialized = false;
};
