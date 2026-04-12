#include "GameplayAbilitySystem/Abilities/ARPGAbilityBase.h"

// ARPGAbilityBase.cpp

#include "GameplayAbilitySystem/Abilities/ARPGAbilityBase.h"

UARPGAbilityBase::UARPGAbilityBase()
{
	InstancingPolicy = EGameplayAbilityInstancingPolicy::InstancedPerActor;
}

void UARPGAbilityBase::PostInitProperties()
{
	Super::PostInitProperties();

	if (!bAbilityInitialized)
	{
		bAbilityInitialized = true;
		InitializeAbility();
	}
}

void UARPGAbilityBase::InitializeAbility()
{
	// ── Identity tag ──────────────────────────
	if (AbilityIdentityTag.IsValid())
	{
		AbilityTags.AddTag(AbilityIdentityTag);
	}

	// ── Activation trigger ────────────────────
	if (ActivationEventTag.IsValid())
	{
		FAbilityTriggerData TriggerData;
		TriggerData.TriggerTag = ActivationEventTag;
		TriggerData.TriggerSource = EGameplayAbilityTriggerSource::GameplayEvent;
		AbilityTriggers.Add(TriggerData);
	}

	// ── Blocked tags ──────────────────────────
	if (BlockedByTags.IsValid())
	{
		ActivationBlockedTags.AppendTags(BlockedByTags);
	}
}
