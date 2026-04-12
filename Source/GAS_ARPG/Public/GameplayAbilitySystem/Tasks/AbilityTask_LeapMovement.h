// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Abilities/Tasks/AbilityTask.h"
#include "AbilityTask_LeapMovement.generated.h"

class ACharacter;
class UGameplayAbility;


UCLASS()
class GAS_ARPG_API UAbilityTask_LeapMovement : public UAbilityTask
{
	DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnLeapCompleted);

	DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnLeapFailed);

	GENERATED_BODY()

public:
	
	UPROPERTY(BlueprintAssignable)
	FOnLeapCompleted OnLeapCompleted;

	UPROPERTY(BlueprintAssignable)
	FOnLeapFailed OnLeapFailed;

	UFUNCTION(BlueprintCallable, Category = "Ability|Tasks",
		meta = (HidePin = "OwningAbility", DefaultToSelf = "OwningAbility"))
	static UAbilityTask_LeapMovement* CreateLeapMovementTask(
		UGameplayAbility* OwningAbility,
		FVector InTargetLocation,
		float InLeapDuration,
		float InArcHeight
	);

	virtual void Activate() override;
	virtual void TickTask(float DeltaTime) override;
	virtual void OnDestroy(bool bInOwnerFinished) override;

private:
	TWeakObjectPtr<ACharacter> OwnerCharacter;

	FVector StartLocation;
	FVector TargetLocation;
	
	float ArcHeight;
	float TotalLeapDuration;
	float ElapsedTime;
};
