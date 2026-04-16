// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "ARPGCharacterBase.h"
#include "EnemyCharacter.generated.h"

struct FAIStimulus;
class UAISenseConfig_Sight;
class UAIPerceptionComponent;
class AEnemyGroupManager;
class UBasicAttributeSet;

UCLASS()
class GAS_ARPG_API AEnemyCharacter : public AARPGCharacterBase
{
	GENERATED_BODY()

public:
	AEnemyCharacter();

	virtual void Tick(float DeltaTime) override;

	virtual void PossessedBy(AController* NewController) override;

	virtual UAbilitySystemComponent* GetAbilitySystemComponent() const override;

	//Manager Functions

	void MoveToLocation(const FVector& Destination) const;

	void MoveInToAttack(const FVector& AttackPosition);

	void FallBackToSlot(const FVector& SlotPosition);

	TWeakObjectPtr<AEnemyGroupManager> GroupManager;

	FVector AssignedSlotPosition = FVector::ZeroVector;

	UAIPerceptionComponent* GetAIPerceptionComponent() const;

	virtual FGenericTeamId GetGenericTeamId() const override
	{
		return FGenericTeamId(static_cast<uint8>(ETeam::Enemy));
	}

protected:
	virtual void BeginPlay() override;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "AbilitySystem")
	TObjectPtr<UBasicAttributeSet> BasicAttributeSet;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "AbilitySystem")
	TObjectPtr<UAbilitySystemComponent> AbilitySystemComponent;

	UPROPERTY(VisibleAnywhere, Category = "AI")
	TObjectPtr<UAIPerceptionComponent> PerceptionComponent;

	UPROPERTY(VisibleAnywhere, Category = "AI")
	TObjectPtr<UAISenseConfig_Sight> SightConfig;

private:
	
	UFUNCTION()
	void OnTargetPerceptionUpdated(AActor* Actor, FAIStimulus Stimulus);

	float MoveUpdateThrottle = 0.f;
};
