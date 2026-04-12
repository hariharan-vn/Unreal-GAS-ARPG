// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "ARPGAbilityBase.h"
#include "LeapSlamAbility.generated.h"

class UGameplayEffect;
class UAbilityTask_LeapMovement;
class AActor;


UCLASS(Blueprintable)
class GAS_ARPG_API ULeapSlamAbility : public UARPGAbilityBase
{
	GENERATED_BODY()

public:
	ULeapSlamAbility();

	virtual bool CanActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo,
	                                const FGameplayTagContainer* SourceTags = nullptr,
	                                const FGameplayTagContainer* TargetTags = nullptr,
	                                OUT FGameplayTagContainer* OptionalRelevantTags = nullptr) const;

protected:
	virtual void ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo,
	                             const FGameplayAbilityActivationInfo ActivationInfo,
	                             const FGameplayEventData* TriggerEventData) override;

	virtual void EndAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo,
	                        const FGameplayAbilityActivationInfo ActivationInfo, bool bReplicateEndAbility,
	                        bool bWasCancelled) override;

	// Called when movement task completes
	UFUNCTION()
	void OnLeapCompleted();

	UFUNCTION()
	void OnLeapFailed();

private:
	float CalculateLeapDuration() const;
	bool ConsumeInstantStartup();

	void ApplyLandingEffects(const FVector& LandingLocation);
	bool IsTargetAtFullLife(const AActor* Target) const;

	float GetDistanceFalloff(const FVector& LandingLocation, const FVector& TargetLocation) const;

	UPROPERTY(EditDefaultsOnly, Category = "LeapSlam|Effects")
	TSubclassOf<UGameplayEffect> GE_Damage;

	UPROPERTY(EditDefaultsOnly, Category = "LeapSlam|Effects")
	TSubclassOf<UGameplayEffect> GE_Stun;

	UPROPERTY(EditDefaultsOnly, Category = "LeapSlam|Effects")
	FGameplayTag DamageAmountTag;

	UPROPERTY(EditDefaultsOnly, Category = "LeapSlam|Damage")
	float BaseDamageMagnitude = 100.f;

	UPROPERTY(EditDefaultsOnly, Category = "LeapSlam|Knockback")
	float MaxKnockbackForce = 1200.f; // force at center

	UPROPERTY(EditDefaultsOnly, Category = "LeapSlam|Knockback")
	float MinKnockbackForce = 300.f; // force at edge

	UPROPERTY(EditDefaultsOnly, Category = "LeapSlam|Knockback")
	float KnockbackUpwardForce = 400.f;

	UPROPERTY(EditDefaultsOnly, Category = "LeapSlam|Movement")
	float ArcHeight = 300.f;

	UPROPERTY(EditDefaultsOnly, Category = "LeapSlam|Movement")
	float LandingRadius = 200.f; //Can increase the radius when buff

	UPROPERTY(EditDefaultsOnly, Category = "LeapSlam|Movement")
	float LeapDistance = 600.f;

	UPROPERTY(EditDefaultsOnly, Category = "LeapSlam|Movement", meta = (ClampMin = "0.0", ClampMax = "1.0"))
	float DamageMinimum = 0.4f;

	UPROPERTY()
	TObjectPtr<UAbilityTask_LeapMovement> ActiveLeapTask;

	float LastLeapTimestamp = -9999.f;
};
