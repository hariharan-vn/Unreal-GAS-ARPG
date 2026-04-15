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
	                                OUT FGameplayTagContainer* OptionalRelevantTags = nullptr) const override;

protected:
	virtual void ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo,
	                             const FGameplayAbilityActivationInfo ActivationInfo,
	                             const FGameplayEventData* TriggerEventData) override;

	virtual void EndAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo,
	                        const FGameplayAbilityActivationInfo ActivationInfo, bool bReplicateEndAbility,
	                        bool bWasCancelled) override;

private:
	UFUNCTION()
	void OnLeapCompleted();

	UFUNCTION()
	void OnLeapFailed();

	UFUNCTION()
	void OnLiftoffNotify(FGameplayEventData Payload);

	UFUNCTION()
	void OnMontageCompleted();

	float CalculateLeapDuration() const;
	bool ConsumeInstantStartup();

	void ApplyLandingDamage(const FVector& LandingLocation);
	bool IsTargetAtFullLife(const AActor* Target) const;

	float GetDistanceFalloff(const FVector& LandingLocation, const FVector& TargetLocation) const;

	FVector GetGroundedTargetLocation(const ACharacter* Avatar) const;

	UPROPERTY(EditDefaultsOnly, Category = "LeapSlam|Effects")
	TSubclassOf<UGameplayEffect> GE_Damage;

	UPROPERTY(EditDefaultsOnly, Category = "LeapSlam|Effects")
	TSubclassOf<UGameplayEffect> GE_Stun;

	UPROPERTY(EditDefaultsOnly, Category = "LeapSlam|Effects")
	TObjectPtr<UAnimMontage> LeapSlamMontage;

	UPROPERTY(EditDefaultsOnly, Category = "LeapSlam|Effects", meta = (Categories = "Event"))
	FGameplayTag LiftoffEventTag;

	UPROPERTY(EditDefaultsOnly, Category = "LeapSlam|Effects", meta = (Categories = "GameplayCue"))
	FGameplayTag LandingCueTag;

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

	UPROPERTY(EditDefaultsOnly, Category = "LeapSlam|Movement", meta = (ClampMin = "0.0", ClampMax = "1.0"))
	float ArcHeightRatio = 0.2f;

	UPROPERTY(EditDefaultsOnly, Category = "LeapSlam|Movement")
	float LandingRadius = 200.f; //Can increase the radius when buff

	UPROPERTY(EditDefaultsOnly, Category = "LeapSlam|Movement")
	float LeapDistance = 600.f;

	UPROPERTY(EditDefaultsOnly, Category = "LeapSlam|Movement", meta = (ClampMin = "0.0", ClampMax = "1.0"))
	float DamageMinimum = 0.4f;

	FVector CachedTargetLocation;

	float CachedDuration;

	UPROPERTY()
	TObjectPtr<UAbilityTask_LeapMovement> ActiveLeapTask;

	bool bLandingCompleted = false;
};
