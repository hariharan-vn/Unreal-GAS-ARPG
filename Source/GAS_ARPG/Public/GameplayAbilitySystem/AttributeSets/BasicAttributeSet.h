// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "AttributeSet.h"
#include "AbilitySystemComponent.h"
#include "BasicAttributeSet.generated.h"

#define ATTRIBUTE_ACCESSORS(ClassName, PropertyName) \
	GAMEPLAYATTRIBUTE_PROPERTY_GETTER(ClassName, PropertyName) \
	GAMEPLAYATTRIBUTE_VALUE_GETTER(PropertyName) \
	GAMEPLAYATTRIBUTE_VALUE_SETTER(PropertyName) \
	GAMEPLAYATTRIBUTE_VALUE_INITTER(PropertyName)

struct FGameplayEffectModCallbackData;

UCLASS()
class GAS_ARPG_API UBasicAttributeSet : public UAttributeSet
{
	GENERATED_BODY()

	DECLARE_MULTICAST_DELEGATE_OneParam(FOnDeath, AActor*);

public:
	UBasicAttributeSet();

	FOnDeath OnDeath;

	//Health Attributes
	UPROPERTY(BlueprintReadOnly, ReplicatedUsing=OnRep_Health, Category = "Attribute")
	FGameplayAttributeData Health;
	ATTRIBUTE_ACCESSORS(UBasicAttributeSet, Health);

	UPROPERTY(BlueprintReadOnly, ReplicatedUsing=OnRep_MaxHealth, Category = "Attribute")
	FGameplayAttributeData MaxHealth;
	ATTRIBUTE_ACCESSORS(UBasicAttributeSet, MaxHealth);

	//Mana Attributes
	UPROPERTY(BlueprintReadOnly, ReplicatedUsing=OnRep_Mana, Category = "Attribute")
	FGameplayAttributeData Mana;
	ATTRIBUTE_ACCESSORS(UBasicAttributeSet, Mana);

	UPROPERTY(BlueprintReadOnly, ReplicatedUsing=OnRep_MaxMana, Category = "Attribute")
	FGameplayAttributeData MaxMana;
	ATTRIBUTE_ACCESSORS(UBasicAttributeSet, MaxMana);

	// ── Weapon Attack Speed ───────────────────────
	// W in LeapSlam formula — set on weapon equip
	// LeapSlam CanActivateAbility returns false if this is 0
	UPROPERTY(BlueprintReadOnly, ReplicatedUsing=OnRep_WeaponAttackSpeed)
	FGameplayAttributeData WeaponAttackSpeed;
	ATTRIBUTE_ACCESSORS(UBasicAttributeSet, WeaponAttackSpeed)

	// ── Global Attack Speed Modifier ─────────────
	// G in LeapSlam formula — modified by buffs/gear
	UPROPERTY(BlueprintReadOnly, ReplicatedUsing=OnRep_GlobalAttackSpeedModifier)
	FGameplayAttributeData GlobalAttackSpeedModifier;
	ATTRIBUTE_ACCESSORS(UBasicAttributeSet, GlobalAttackSpeedModifier)

	UFUNCTION()
	void OnRep_Health(const FGameplayAttributeData& OldValue) const
	{
		GAMEPLAYATTRIBUTE_REPNOTIFY(UBasicAttributeSet, Health, OldValue);
	}

	UFUNCTION()
	void OnRep_MaxHealth(const FGameplayAttributeData& OldValue) const
	{
		GAMEPLAYATTRIBUTE_REPNOTIFY(UBasicAttributeSet, MaxHealth, OldValue);
	}

	UFUNCTION()
	void OnRep_Mana(const FGameplayAttributeData& OldValue) const
	{
		GAMEPLAYATTRIBUTE_REPNOTIFY(UBasicAttributeSet, Mana, OldValue);
	}

	UFUNCTION()
	void OnRep_MaxMana(const FGameplayAttributeData& OldValue) const
	{
		GAMEPLAYATTRIBUTE_REPNOTIFY(UBasicAttributeSet, MaxMana, OldValue);
	}

	UFUNCTION()
	void OnRep_WeaponAttackSpeed(const FGameplayAttributeData& OldValue) const
	{
		GAMEPLAYATTRIBUTE_REPNOTIFY(UBasicAttributeSet, WeaponAttackSpeed, OldValue);
	}

	UFUNCTION()
	void OnRep_GlobalAttackSpeedModifier(const FGameplayAttributeData& OldValue) const
	{
		GAMEPLAYATTRIBUTE_REPNOTIFY(UBasicAttributeSet, GlobalAttackSpeedModifier, OldValue);
	}

	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	virtual void PreAttributeChange(const FGameplayAttribute& Attribute, float& NewValue) override;

	virtual void PostGameplayEffectExecute(const FGameplayEffectModCallbackData& Data) override;

private:
	void OnHealthDepleted() const;

	FGameplayTag DeathTag;
};
