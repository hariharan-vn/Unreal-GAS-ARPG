// Fill out your copyright notice in the Description page of Project Settings.


#include "GameplayAbilitySystem/Abilities/LeapSlamAbility.h"

#include "GameFramework/Character.h"
#include "GameplayAbilitySystem/Tasks/AbilityTask_LeapMovement.h"
#include "GAS_ARPG/GAS_ARPG.h"
#include "AbilitySystemBlueprintLibrary.h"
#include "AbilitySystemComponent.h"
#include "Engine/OverlapResult.h"
#include "GameplayAbilitySystem/AttributeSets/BasicAttributeSet.h"

ULeapSlamAbility::ULeapSlamAbility()
{
}

bool ULeapSlamAbility::CanActivateAbility(const FGameplayAbilitySpecHandle Handle,
                                          const FGameplayAbilityActorInfo* ActorInfo,
                                          const FGameplayTagContainer* SourceTags,
                                          const FGameplayTagContainer* TargetTags,
                                          FGameplayTagContainer* OptionalRelevantTags) const
{
	if (!Super::CanActivateAbility(Handle, ActorInfo,
	                               SourceTags, TargetTags,
	                               OptionalRelevantTags))
		return false;

	const UAbilitySystemComponent* ASC = ActorInfo->AbilitySystemComponent.Get();

	if (!ASC->HasMatchingGameplayTag(FGameplayTag::RequestGameplayTag("Weapon.Equipped")))
	{
		UE_LOG(ARPG_Ability, Warning, TEXT("[LeapSlam] No weapon equipped"));
		return false;
	}

	return true;
}


void ULeapSlamAbility::ActivateAbility(const FGameplayAbilitySpecHandle Handle,
                                       const FGameplayAbilityActorInfo* ActorInfo,
                                       const FGameplayAbilityActivationInfo ActivationInfo,
                                       const FGameplayEventData* TriggerEventData)
{
	Super::ActivateAbility(Handle, ActorInfo, ActivationInfo, TriggerEventData);

	if (!CommitAbility(Handle, ActorInfo, ActivationInfo))
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		UE_LOG(ARPG_Ability, Warning, TEXT("[%hs] Failed on CommitAbility"), __FUNCTION__);
		return;
	}

	const bool bInstant = ConsumeInstantStartup();

	if (!bInstant)
	{
		// TODO: Play windup montage section here before proceeding
		UE_LOG(ARPG_Ability, Verbose, TEXT("[%hs] InstantStartup: %s"), __FUNCTION__,
		       bInstant ? TEXT("true") : TEXT("false"));
		// For now we fall through — wire montage delegate to continue
	}

	const ACharacter* Avatar = Cast<ACharacter>(ActorInfo->AvatarActor.Get());
	if (!Avatar)
	{
		UE_LOG(ARPG_Ability, Warning, TEXT("[%hs] Unable to Get the Avatar"), __FUNCTION__);
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}
	const FVector TargetLocation = Avatar->GetActorLocation() + Avatar->GetActorForwardVector() * LeapDistance;

	// ── Create & Start Leap Task ──────────────
	const float Duration = CalculateLeapDuration();

	ActiveLeapTask = UAbilityTask_LeapMovement::CreateLeapMovementTask(this, TargetLocation, Duration, ArcHeight);

	ActiveLeapTask->OnLeapCompleted.AddDynamic(this, &ULeapSlamAbility::OnLeapCompleted);
	ActiveLeapTask->OnLeapFailed.AddDynamic(this, &ULeapSlamAbility::OnLeapFailed);
	ActiveLeapTask->ReadyForActivation();
}

void ULeapSlamAbility::EndAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo,
                                  const FGameplayAbilityActivationInfo ActivationInfo, bool bReplicateEndAbility,
                                  bool bWasCancelled)
{
	ActiveLeapTask = nullptr;
	Super::EndAbility(Handle, ActorInfo, ActivationInfo, bReplicateEndAbility, bWasCancelled);
}

void ULeapSlamAbility::OnLeapCompleted()
{
	const FGameplayAbilityActorInfo* ActorInfo = GetCurrentActorInfo();

	if (const ACharacter* Avatar = Cast<ACharacter>(ActorInfo->AvatarActor.Get()))
	{
		ApplyLandingEffects(Avatar->GetActorLocation());
	}
	EndAbility(CurrentSpecHandle, ActorInfo, CurrentActivationInfo, true, false);
}

void ULeapSlamAbility::OnLeapFailed()
{
	EndAbility(CurrentSpecHandle, GetCurrentActorInfo(),
	           CurrentActivationInfo, true, true);
}

float ULeapSlamAbility::CalculateLeapDuration() const
{
	// https://www.poewiki.net/wiki/Leap_Slam#Skill_functions_and_interactions
	// (1 + G) × (1 / W) + 0.55
	// TODO : Fetch the required data here
	const float G = 0.f; // replace with actual global attack speed modifier
	const float W = 1.f; // replace with weapon attack speed

	return (1.f + G) * (1 / ((1.f / W) + 0.55f));
}

bool ULeapSlamAbility::ConsumeInstantStartup()
{
	const float Now = GetWorld()->GetTimeSeconds();
	const bool bInstant = (Now - LastLeapTimestamp) >= CalculateLeapDuration();
	LastLeapTimestamp = Now;
	return bInstant;
}

void ULeapSlamAbility::ApplyLandingEffects(const FVector& LandingLocation)
{
	TArray<FOverlapResult> Overlaps;
	FCollisionQueryParams Params;
	Params.AddIgnoredActor(GetAvatarActorFromActorInfo());

	GetWorld()->OverlapMultiByChannel(
		Overlaps,
		LandingLocation,
		FQuat::Identity,
		ECC_Pawn,
		FCollisionShape::MakeSphere(LandingRadius),
		Params
	);

	for (const FOverlapResult& Overlap : Overlaps)
	{
		AActor* Target = Overlap.GetActor();
		if (!Target) continue;

		const float Falloff = GetDistanceFalloff(LandingLocation, Target->GetActorLocation());
		const float ScaledDamage = FMath::Lerp(BaseDamageMagnitude * DamageMinimum, BaseDamageMagnitude, Falloff);

		const FGameplayEffectSpecHandle DamageSpec = MakeOutgoingGameplayEffectSpec(GE_Damage);
		DamageSpec.Data->SetSetByCallerMagnitude(DamageAmountTag, ScaledDamage);

		ApplyGameplayEffectSpecToTarget(CurrentSpecHandle, GetCurrentActorInfo(), CurrentActivationInfo, DamageSpec,
		                                UAbilitySystemBlueprintLibrary::AbilityTargetDataFromActor(Target));

		// ── Knockback ─────────────────────────
		if (ACharacter* TargetChar = Cast<ACharacter>(Target))
		{
			const FVector KnockDir = (Target->GetActorLocation() - LandingLocation).GetSafeNormal();
			const float KnockForce = FMath::Lerp(MinKnockbackForce, MaxKnockbackForce, Falloff);
			const FVector LaunchVelocity = KnockDir * KnockForce + FVector(0.f, 0.f, KnockbackUpwardForce * Falloff);
			TargetChar->LaunchCharacter(LaunchVelocity, true, true);
		}

		// ── Stun if Full Life ─────────────────
		if (IsTargetAtFullLife(Target))
		{
			const FGameplayEffectSpecHandle StunSpec = MakeOutgoingGameplayEffectSpec(GE_Stun);
			ApplyGameplayEffectSpecToTarget(CurrentSpecHandle, GetCurrentActorInfo(), CurrentActivationInfo, StunSpec,
			                                UAbilitySystemBlueprintLibrary::AbilityTargetDataFromActor(Target));
		}
	}
}

float ULeapSlamAbility::GetDistanceFalloff(const FVector& LandingLocation, const FVector& TargetLocation) const
{
	const float Distance = FVector::Dist2D(LandingLocation, TargetLocation);
	return FMath::Clamp(1.f - (Distance / LandingRadius), 0.f, 1.f);
}

bool ULeapSlamAbility::IsTargetAtFullLife(const AActor* Target) const
{
	const UAbilitySystemComponent* AbilitySystemComponent = Target->FindComponentByClass<UAbilitySystemComponent>();
	if (!AbilitySystemComponent) return false;

	const float HP = AbilitySystemComponent->GetNumericAttribute(UBasicAttributeSet::GetHealthAttribute());
	const float MaxHP = AbilitySystemComponent->GetNumericAttribute(UBasicAttributeSet::GetMaxHealthAttribute());

	return FMath::IsNearlyEqual(HP, MaxHP, 1.f);
}
