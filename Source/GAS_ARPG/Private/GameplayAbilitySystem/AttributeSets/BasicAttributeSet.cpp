// Fill out your copyright notice in the Description page of Project Settings.


#include "GameplayAbilitySystem/AttributeSets/BasicAttributeSet.h"
#include "GameplayEffectExtension.h"
#include "Net/UnrealNetwork.h"


UBasicAttributeSet::UBasicAttributeSet()
{
}

void UBasicAttributeSet::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME_CONDITION_NOTIFY(UBasicAttributeSet, Health, COND_None, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(UBasicAttributeSet, MaxHealth, COND_None, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(UBasicAttributeSet, Mana, COND_None, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(UBasicAttributeSet, MaxMana, COND_None, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(UBasicAttributeSet, WeaponAttackSpeed, COND_None, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(UBasicAttributeSet, GlobalAttackSpeedModifier, COND_None, REPNOTIFY_Always);
}

void UBasicAttributeSet::PreAttributeChange(const FGameplayAttribute& Attribute, float& NewValue)
{
	Super::PreAttributeChange(Attribute, NewValue);

	if (Attribute == GetHealthAttribute())
	{
		NewValue = FMath::Clamp(NewValue, 0.0f, GetMaxHealth());
	}
	else if (Attribute == GetManaAttribute())
	{
		NewValue = FMath::Clamp(NewValue, 0.0f, GetMaxMana());
	}
	// G must stay above -0.9 — below that the formula breaks
	// (1/W) / (1 + G) → divide by zero if G = -1
	else if (Attribute == GetGlobalAttackSpeedModifierAttribute())
	{
		NewValue = FMath::Max(NewValue, -0.9f);
	}
	// W must stay positive — it's a divisor in the LeapSlam formula
	else if (Attribute == GetWeaponAttackSpeedAttribute())
	{
		NewValue = FMath::Max(NewValue, 0.f);
	}
}

void UBasicAttributeSet::PostGameplayEffectExecute(const FGameplayEffectModCallbackData& Data)
{
	Super::PostGameplayEffectExecute(Data);

	if (Data.EvaluatedData.Attribute == GetHealthAttribute())
	{
		if (Data.EvaluatedData.Attribute == GetHealthAttribute())
		{
			SetHealth(FMath::Clamp(GetHealth(), 0.f, GetMaxHealth()));

			if (GetHealth() <= 0.f)
			{
				OnHealthDepleted();
			}
		}
	}
	else if (Data.EvaluatedData.Attribute == GetManaAttribute())
	{
		SetMana(FMath::Clamp(GetMana(), 0.f, GetMaxMana()));
	}
}

void UBasicAttributeSet::OnHealthDepleted() const
{
	// Get the owning ASC and apply death tag
	UAbilitySystemComponent* ASC = GetOwningAbilitySystemComponent();
	if (!ASC) return;

	// Prevent multiple death triggers
	if (ASC->HasMatchingGameplayTag(FGameplayTag::RequestGameplayTag("State.Dead")))
		return;

	UE_LOG(LogTemp, Warning, TEXT("[BasicAttributeSet] %s has died"), *GetOwningActor()->GetName());

	// Apply death tag — AI, animations, abilities all
	// check this tag to respond to death
	FGameplayTagContainer DeathTag;
	DeathTag.AddTag(FGameplayTag::RequestGameplayTag("State.Dead"));
	ASC->AddLooseGameplayTags(DeathTag);

	// Broadcast so character class can play death montage
	// ragdoll, disable input etc
	OnDeath.Broadcast(GetOwningActor());
}
