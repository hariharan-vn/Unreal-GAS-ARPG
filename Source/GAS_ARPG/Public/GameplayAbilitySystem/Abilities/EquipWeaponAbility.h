// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "ARPGAbilityBase.h"
#include "Items/Weapon/WeaponData.h"
#include "EquipWeaponAbility.generated.h"

struct FWeaponData;
class UAbilityTask_PlayMontageAndWait;
class UGameplayEffect;
class AWeaponActor;

UCLASS()
class GAS_ARPG_API UEquipWeaponAbility : public UARPGAbilityBase
{
	GENERATED_BODY()

public:
	UEquipWeaponAbility();

protected:
	virtual void ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo,
	                             const FGameplayAbilityActivationInfo ActivationInfo,
	                             const FGameplayEventData* TriggerEventData) override;

	virtual void EndAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo,
	                        const FGameplayAbilityActivationInfo ActivationInfo, bool bReplicateEndAbility,
	                        bool bWasCancelled) override;

private:
	UFUNCTION()
	void OnEquipMontageCompleted();

	UFUNCTION()
	void OnEquipMontageCancelled();

	void ApplyWeaponGE();
	void RemovePreviousWeaponGE();
	void SpawnAndAttachWeaponActor();
	void RemovePreviousWeaponActor();

	// ── Single GE for all weapons ─────────────
	// Value driven by SetByCaller — not per weapon
	UPROPERTY(EditDefaultsOnly, Category = "Equip")
	TSubclassOf<UGameplayEffect> GE_WeaponStats;

	UPROPERTY(EditDefaultsOnly, Category = "Equip")
	TObjectPtr<UAnimMontage> EquipMontage;

	// ── Runtime State ─────────────────────────
	// Safe on ability — InstancedPerActor
	FActiveGameplayEffectHandle ActiveWeaponGEHandle;
	FWeaponData PendingWeaponData;

	UPROPERTY(EditDefaultsOnly, Category = "Equip")
	FGameplayTag WeaponAttackSpeedDataTag;

	UPROPERTY(EditDefaultsOnly, Category = "Equip")
	FGameplayTag WeaponSpawnedTag;
	
	UPROPERTY()
	TObjectPtr<AWeaponActor> SpawnedWeaponActor;

	UPROPERTY()
	TObjectPtr<UAbilityTask_PlayMontageAndWait> MontageTask;
};
