// Fill out your copyright notice in the Description page of Project Settings.


#include "GameplayAbilitySystem/Abilities/LeapSlamAbility.h"

#include "GameFramework/Character.h"
#include "GameplayAbilitySystem/Tasks/AbilityTask_LeapMovement.h"
#include "GAS_ARPG/GAS_ARPG.h"
#include "AbilitySystemBlueprintLibrary.h"
#include "AbilitySystemComponent.h"
#include "Abilities/Tasks/AbilityTask_PlayMontageAndWait.h"
#include "Abilities/Tasks/AbilityTask_WaitGameplayEvent.h"
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

	const UAbilitySystemComponent* ASC = GetAbilitySystemComponentFromActorInfo();

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

	const ACharacter* Avatar = Cast<ACharacter>(ActorInfo->AvatarActor.Get());
	if (!Avatar)
	{
		UE_LOG(ARPG_Ability, Warning, TEXT("[%hs] Unable to Get the Avatar"), __FUNCTION__);
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}

	bLandingCompleted = false;
	CachedTargetLocation = GetGroundedTargetLocation(Avatar);
	CachedDuration = CalculateLeapDuration();

	UAbilityTask_PlayMontageAndWait* MontageTask = UAbilityTask_PlayMontageAndWait::CreatePlayMontageAndWaitProxy(
		this, NAME_None, LeapSlamMontage, 1.f, FName("Start"));
	MontageTask->OnCompleted.AddDynamic(this, &ULeapSlamAbility::OnMontageCompleted);
	MontageTask->OnInterrupted.AddDynamic(this, &ULeapSlamAbility::OnLeapFailed);
	MontageTask->ReadyForActivation();

	UAbilityTask_WaitGameplayEvent* LiftoffTask = UAbilityTask_WaitGameplayEvent::WaitGameplayEvent(
		this, LiftoffEventTag);
	LiftoffTask->EventReceived.AddDynamic(this, &ULeapSlamAbility::OnLiftoffNotify);
	LiftoffTask->ReadyForActivation();
}

void ULeapSlamAbility::EndAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo,
                                  const FGameplayAbilityActivationInfo ActivationInfo, bool bReplicateEndAbility,
                                  bool bWasCancelled)
{
	ActiveLeapTask = nullptr;
	bLandingCompleted = false;
	CachedTargetLocation = FVector::ZeroVector;
	CachedDuration = 0.f;
	Super::EndAbility(Handle, ActorInfo, ActivationInfo, bReplicateEndAbility, bWasCancelled);
}

void ULeapSlamAbility::OnLiftoffNotify(FGameplayEventData Payload)
{
	if (LeapSlamMontage)
	{
		const float LoopLength = LeapSlamMontage->GetSectionLength(
			LeapSlamMontage->GetSectionIndex(FName("Loop")));
		const float DesiredPlayRate = LoopLength / CachedDuration;

		GetAbilitySystemComponentFromActorInfo()->CurrentMontageSetPlayRate(DesiredPlayRate);
		GetAbilitySystemComponentFromActorInfo()->CurrentMontageJumpToSection(FName("Loop"));
	}

	// Now start movement
	ActiveLeapTask = UAbilityTask_LeapMovement::CreateLeapMovementTask(
		this, CachedTargetLocation, CachedDuration, ArcHeightRatio);
	ActiveLeapTask->OnLeapCompleted.AddDynamic(this, &ULeapSlamAbility::OnLeapCompleted);
	ActiveLeapTask->OnLeapFailed.AddDynamic(this, &ULeapSlamAbility::OnLeapFailed);
	ActiveLeapTask->ReadyForActivation();
}

void ULeapSlamAbility::OnLeapCompleted()
{
	bLandingCompleted = true;

	GetAbilitySystemComponentFromActorInfo()->CurrentMontageSetPlayRate(1.f);

	GetAbilitySystemComponentFromActorInfo()->CurrentMontageJumpToSection(FName("End"));

	const FGameplayAbilityActorInfo* ActorInfo = GetCurrentActorInfo();

	const FVector LandingLocation =
		GetAvatarActorFromActorInfo()->GetActorLocation();

	ApplyLandingDamage(LandingLocation);

	FGameplayCueParameters CueParams;
	CueParams.Location = LandingLocation;
	CueParams.NormalizedMagnitude = 1.f;

	GetAbilitySystemComponentFromActorInfo()->ExecuteGameplayCue(LandingCueTag, CueParams);
}

void ULeapSlamAbility::OnMontageCompleted()
{
	if (!bLandingCompleted)
	{
		//Not yet reached landing state
		return;
	}

	EndAbility(CurrentSpecHandle, GetCurrentActorInfo(), CurrentActivationInfo, true, false);
}

void ULeapSlamAbility::OnLeapFailed()
{
	EndAbility(CurrentSpecHandle, GetCurrentActorInfo(),
	           CurrentActivationInfo, true, true);
}

float ULeapSlamAbility::CalculateLeapDuration() const
{
	// https://www.poewiki.net/wiki/Leap_Slam#Skill_functions_and_interactions
	// Duration = (1/W + 0.55) / (1+G) 
	const UAbilitySystemComponent* ASC = GetAbilitySystemComponentFromActorInfo();
	if (!ASC)
	{
		UE_LOG(ARPG_Ability, Warning, TEXT("[%hs] ASC not found, using fallback duration"), __FUNCTION__);
		return 1.f;
	}

	const UBasicAttributeSet* Attrs = ASC->GetSet<UBasicAttributeSet>();
	if (!Attrs)
	{
		UE_LOG(ARPG_Ability, Warning, TEXT("[%hs] AttributeSet not found, using fallback duration"), __FUNCTION__);
		return 1.f;
	}

	const float W = FMath::Max(Attrs->GetWeaponAttackSpeed(), KINDA_SMALL_NUMBER); // guard against div/0
	const float G = Attrs->GetGlobalAttackSpeedModifier();

	return (1.f / W + 0.55f) / (1.f + G);
}


void ULeapSlamAbility::ApplyLandingDamage(const FVector& LandingLocation)
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

FVector ULeapSlamAbility::GetGroundedTargetLocation(const ACharacter* Avatar) const
{
	APlayerController* PC = Avatar->GetController<APlayerController>();
	if (!PC) return Avatar->GetActorLocation();

	// Trace from camera to world
	FHitResult CursorHit;
	PC->GetHitResultUnderCursor(ECC_WorldStatic, false, CursorHit);

	FVector RawTarget = CursorHit.bBlockingHit
		                    ? CursorHit.Location
		                    : Avatar->GetActorLocation() + Avatar->GetActorForwardVector() * LeapDistance;

	// Clamp to max leap distance
	const FVector ToTarget = RawTarget - Avatar->GetActorLocation();
	if (ToTarget.SizeSquared2D() > FMath::Square(LeapDistance))
	{
		RawTarget = Avatar->GetActorLocation() + ToTarget.GetSafeNormal2D() * LeapDistance;
	}

	// Ground trace
	FHitResult GroundHit;
	const FVector TraceStart = RawTarget + FVector(0.f, 0.f, 500.f);
	const FVector TraceEnd = RawTarget - FVector(0.f, 0.f, 1000.f);
	GetWorld()->LineTraceSingleByChannel(GroundHit, TraceStart, TraceEnd, ECC_WorldStatic);

	return GroundHit.bBlockingHit ? GroundHit.Location : RawTarget;
}
