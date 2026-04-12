// Fill out your copyright notice in the Description page of Project Settings.


#include "GameplayAbilitySystem/Abilities/EquipWeaponAbility.h"

#include "Abilities/Tasks/AbilityTask_PlayMontageAndWait.h"
#include "AbilitySystemComponent.h"
#include "GameFramework/Character.h"
#include "GAS_ARPG/GAS_ARPG.h"
#include "Items/Weapon/WeaponActor.h"
#include "Items/Weapon/WeaponData.h"
#include "Items/Weapon/WeaponPickupPayload.h"

UEquipWeaponAbility::UEquipWeaponAbility()
{
}

void UEquipWeaponAbility::ActivateAbility(const FGameplayAbilitySpecHandle Handle,
                                          const FGameplayAbilityActorInfo* ActorInfo,
                                          const FGameplayAbilityActivationInfo ActivationInfo,
                                          const FGameplayEventData* TriggerEventData)
{
	Super::ActivateAbility(Handle, ActorInfo, ActivationInfo, TriggerEventData);

	if (!CommitAbility(Handle, ActorInfo, ActivationInfo))
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}

	if (!TriggerEventData)
	{
		UE_LOG(ARPG_Ability, Warning, TEXT("[EquipWeapon] No TriggerEventData"));
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}

	// ── Read WeaponData from pickup ───────────
	const UWeaponPickupPayload* Payload = Cast<UWeaponPickupPayload>(TriggerEventData->OptionalObject.Get());
	if (!Payload)
	{
		UE_LOG(ARPG_Ability, Warning,
		       TEXT("[EquipWeapon] OptionalObject is not UWeaponPickupPayload"));
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}

	// Copy data before destroying pickup
	PendingWeaponData = Payload->GetWeaponData();

	// ── Play montage or apply immediately ─────
	if (EquipMontage)
	{
		MontageTask = UAbilityTask_PlayMontageAndWait::CreatePlayMontageAndWaitProxy(this, NAME_None, EquipMontage);
		MontageTask->OnCompleted.AddDynamic(this, &UEquipWeaponAbility::OnEquipMontageCompleted);
		MontageTask->OnCancelled.AddDynamic(this, &UEquipWeaponAbility::OnEquipMontageCancelled);
		MontageTask->ReadyForActivation();
	}
	else
	{
		// No montage configured — apply immediately
		OnEquipMontageCompleted();
	}
}

void UEquipWeaponAbility::OnEquipMontageCompleted()
{
	RemovePreviousWeaponGE();
	RemovePreviousWeaponActor();
	ApplyWeaponGE();
	SpawnAndAttachWeaponActor();

	EndAbility(CurrentSpecHandle, GetCurrentActorInfo(), CurrentActivationInfo, true, false);
}

void UEquipWeaponAbility::OnEquipMontageCancelled()
{
	// Data was already copied — weapon not applied yet
	// Nothing to clean up except ending the ability
	EndAbility(CurrentSpecHandle, GetCurrentActorInfo(), CurrentActivationInfo, true, true);
}

void UEquipWeaponAbility::RemovePreviousWeaponGE()
{
	UAbilitySystemComponent* ASC = GetAbilitySystemComponentFromActorInfo();
	if (!ASC) return;

	if (ActiveWeaponGEHandle.IsValid())
	{
		ASC->RemoveActiveGameplayEffect(ActiveWeaponGEHandle);
		ActiveWeaponGEHandle.Invalidate();
	}
}

void UEquipWeaponAbility::ApplyWeaponGE()
{
	UAbilitySystemComponent* ASC = GetAbilitySystemComponentFromActorInfo();
	if (!ASC || !GE_WeaponStats) return;

	FGameplayEffectContextHandle Context = ASC->MakeEffectContext();
	Context.AddSourceObject(GetAvatarActorFromActorInfo());

	const FGameplayEffectSpecHandle Spec = ASC->MakeOutgoingSpec(GE_WeaponStats, 1.f, Context);
	if (!Spec.IsValid()) return;

	// ── Value via SetByCaller ─────────────────
	// GE_WeaponStats reads this tag to set WeaponAttackSpeed
	Spec.Data->SetSetByCallerMagnitude(FGameplayTag::RequestGameplayTag("Data.WeaponAttackSpeed"),
	                                   PendingWeaponData.WeaponAttackSpeed);

	// ── Identity tag via DynamicGrantedTags ───
	// Grants "Weapon.Type.Sword" etc while GE is active
	// Removed automatically when weapon is unequipped
	if (PendingWeaponData.WeaponTypeTag.IsValid())
	{
		Spec.Data->DynamicGrantedTags.AddTag(PendingWeaponData.WeaponTypeTag);
	}

	ActiveWeaponGEHandle = ASC->ApplyGameplayEffectSpecToSelf(*Spec.Data.Get());
}

void UEquipWeaponAbility::SpawnAndAttachWeaponActor()
{
	if (!PendingWeaponData.WeaponActorClass) return;

	ACharacter* Avatar = Cast<ACharacter>(GetAvatarActorFromActorInfo());
	if (!Avatar) return;

	FActorSpawnParameters Params;
	Params.Owner = Avatar;

	SpawnedWeaponActor = GetWorld()->SpawnActor<AWeaponActor>(PendingWeaponData.WeaponActorClass, FTransform::Identity,
	                                                          Params);

	if (SpawnedWeaponActor)
	{
		SpawnedWeaponActor->AttachToComponent(Avatar->GetMesh(),
		                                      FAttachmentTransformRules::SnapToTargetNotIncludingScale,
		                                      PendingWeaponData.SocketName);
	}
}

void UEquipWeaponAbility::RemovePreviousWeaponActor()
{
	if (SpawnedWeaponActor)
	{
		SpawnedWeaponActor->Destroy();
		SpawnedWeaponActor = nullptr;
	}
}

void UEquipWeaponAbility::EndAbility(const FGameplayAbilitySpecHandle Handle,
                                     const FGameplayAbilityActorInfo* ActorInfo,
                                     const FGameplayAbilityActivationInfo ActivationInfo, bool bReplicateEndAbility,
                                     bool bWasCancelled)
{
	MontageTask = nullptr;
	Super::EndAbility(Handle, ActorInfo, ActivationInfo, bReplicateEndAbility, bWasCancelled);
}
