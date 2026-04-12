#include "GameplayAbilitySystem/Abilities/BasicAttackAbility.h"
#include "Abilities/Tasks/AbilityTask_PlayMontageAndWait.h"
#include "Abilities/Tasks/AbilityTask_WaitGameplayEvent.h"
#include "AbilitySystemComponent.h"
#include "AbilitySystemBlueprintLibrary.h"
#include "GameplayAbilitySystem/Tasks/AbilityTask_TickTrace.h"
#include "GAS_ARPG/GAS_ARPG.h"

UBasicAttackAbility::UBasicAttackAbility()
{
}

void UBasicAttackAbility::ActivateAbility(const FGameplayAbilitySpecHandle Handle,
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

	if (!AttackMontage)
	{
		UE_LOG(ARPG_Ability, Warning, TEXT("[BasicAttack] No montage set"));
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}

	// ── Listen for hit window open ─────────────
	// AN_SendGameplayEvent on montage fires Event.Attack.OpenHitWindow
	OpenWindowTask = UAbilityTask_WaitGameplayEvent::WaitGameplayEvent(
		this, FGameplayTag::RequestGameplayTag("Event.Attack.OpenHitWindow"));
	OpenWindowTask->EventReceived.AddDynamic(this, &UBasicAttackAbility::OnHitWindowOpen);
	OpenWindowTask->ReadyForActivation();

	// ── Listen for hit window close ────────────
	CloseWindowTask = UAbilityTask_WaitGameplayEvent::WaitGameplayEvent(
		this, FGameplayTag::RequestGameplayTag("Event.Attack.CloseHitWindow"));
	CloseWindowTask->EventReceived.AddDynamic(this, &UBasicAttackAbility::OnHitWindowClose);
	CloseWindowTask->ReadyForActivation();

	// ── Play attack montage ────────────────────
	MontageTask = UAbilityTask_PlayMontageAndWait::CreatePlayMontageAndWaitProxy(this, NAME_None, AttackMontage);
	MontageTask->OnCompleted.AddDynamic(this, &UBasicAttackAbility::OnMontageCompleted);
	MontageTask->OnCancelled.AddDynamic(this, &UBasicAttackAbility::OnMontageCancelled);
	MontageTask->ReadyForActivation();
}

// ── Hit Window ────────────────────────────────

void UBasicAttackAbility::OnHitWindowOpen(FGameplayEventData Payload)
{
	TraceTask = UAbilityTask_TickTrace::CreateTickTrace(this, WeaponBaseSocket, WeaponTipSocket, SweepRadius);
	TraceTask->OnHit.AddDynamic(this, &UBasicAttackAbility::OnTraceHit);
	TraceTask->ReadyForActivation();
}

void UBasicAttackAbility::OnHitWindowClose(FGameplayEventData Payload)
{
	if (TraceTask)
	{
		TraceTask->StopTrace();
		TraceTask = nullptr;
	}
}

void UBasicAttackAbility::ApplyDamageToTarget(AActor* Target) const
{
	if (!GE_Damage) return;

	const FGameplayEffectSpecHandle Spec = MakeOutgoingGameplayEffectSpec(GE_Damage);

	if (!Spec.IsValid()) return;

	// Negative value — Health | Add | -DamageAmount
	Spec.Data->SetSetByCallerMagnitude(DamageAmountTag, -DamageAmount);

	ApplyGameplayEffectSpecToTarget(CurrentSpecHandle, GetCurrentActorInfo(), CurrentActivationInfo, Spec,
	                                UAbilitySystemBlueprintLibrary::AbilityTargetDataFromActor(Target));

	UE_LOG(ARPG_Ability, Verbose, TEXT("[BasicAttack] Hit %s for %.1f damage"), *Target->GetName(), DamageAmount);
}

void UBasicAttackAbility::OnTraceHit(const FHitResult& Hit)
{
	AActor* Target = Hit.GetActor();
	if (!Target) return;

	ApplyDamageToTarget(Target);
}

void UBasicAttackAbility::OnMontageCompleted()
{
	EndAbility(CurrentSpecHandle, GetCurrentActorInfo(),
	           CurrentActivationInfo, true, false);
}

void UBasicAttackAbility::OnMontageCancelled()
{
	EndAbility(CurrentSpecHandle, GetCurrentActorInfo(),
	           CurrentActivationInfo, true, true);
}

void UBasicAttackAbility::EndAbility(const FGameplayAbilitySpecHandle Handle,
                                     const FGameplayAbilityActorInfo* ActorInfo,
                                     const FGameplayAbilityActivationInfo ActivationInfo, bool bReplicateEndAbility,
                                     bool bWasCancelled)
{
	if (TraceTask)
	{
		TraceTask->StopTrace();
		TraceTask = nullptr;
	}

	MontageTask = nullptr;
	OpenWindowTask = nullptr;
	CloseWindowTask = nullptr;

	Super::EndAbility(Handle, ActorInfo, ActivationInfo,
	                  bReplicateEndAbility, bWasCancelled);
}
