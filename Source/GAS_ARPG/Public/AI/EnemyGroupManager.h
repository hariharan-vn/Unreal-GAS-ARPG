// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "EnemyGroupManager.generated.h"


class AEnemyCharacter;

UENUM(BlueprintType)
enum class EGroupState : uint8
{
	Idle,
	Investigating,
	Circling,
	Halting,
	AttackWindup,
	Attacking,
	AttackRecovery
};

UCLASS()
class GAS_ARPG_API AEnemyGroupManager : public AActor
{
	GENERATED_BODY()

public:
	AEnemyGroupManager();

	virtual void Tick(float DeltaTime) override;

	// Called by enemies when they detect/lose player
	void OnEnemyDetectedPlayer(AEnemyCharacter* Enemy, AActor* Player);

	void OnEnemyLostPlayer(const AEnemyCharacter* Enemy, AActor* Player);

	// Called when an enemy dies — recalculate slots
	void OnEnemyDied(const AEnemyCharacter* Enemy);

	TWeakObjectPtr<AActor> GetTrackedPlayer() const { return TrackedPlayer; }

protected:
	virtual void BeginPlay() override;

	UPROPERTY(EditAnywhere, Category = "Group|Setup")
	TArray<TObjectPtr<AEnemyCharacter>> RegisteredEnemies; //Assigned in the Editor

private:
	// ── Internal ─────────────────────────────
	void SetGroupState(EGroupState NewState);
	void UpdateCirclePositions();
	void UpdateAttacking(float DeltaTime);
	void SelectNextAttacker();
	bool IsPlayerWithinEngageRadius() const;
	bool IsPlayerBeyondBreakRadius() const;
	TArray<AEnemyCharacter*> GetAliveEnemies() const;
	void HandleState(const float DeltaTime);
	void StopAllEnemies() const;

	TWeakObjectPtr<AActor> TrackedPlayer;

	EGroupState CurrentState = EGroupState::Idle;

	// Currently assigned attacker
	TWeakObjectPtr<AEnemyCharacter> CurrentAttacker;

	UPROPERTY(EditAnywhere, Category = "Group|Radius")
	float EngageRadius = 800.f; // player enters → start circling

	UPROPERTY(EditAnywhere, Category = "Group|Radius")
	float BreakRadius = 1500.f; // player exits → return to idle

	UPROPERTY(EditAnywhere, Category = "Group|Radius")
	float CircleRadius = 400.f; // orbit distance from player

	UPROPERTY(EditAnywhere, Category = "Group|Radius")
	float AttackRange = 150.f; // how close attacker gets before falling back

	// ── Attack Selection ─────────────────────
	UPROPERTY(EditAnywhere, Category = "Group|Attack")
	float MinAttackInterval = 2.f;

	UPROPERTY(EditAnywhere, Category = "Group|Attack")
	float MaxAttackInterval = 4.f;

	// How long enemies circle before halting
	UPROPERTY(EditAnywhere, Category = "Group|Timing")
	float CircleDuration = 3.f;

	// How long enemies halt before attack begins
	UPROPERTY(EditAnywhere, Category = "Group|Timing")
	float HaltDuration = 1.f;

	// How long attacker winds up before moving in
	UPROPERTY(EditAnywhere, Category = "Group|Timing")
	float AttackWindupDuration = 0.8f;

	// How long after attack before circling resumes
	UPROPERTY(EditAnywhere, Category = "Group|Timing")
	float AttackRecoveryDuration = 1.5f;

	float AttackTimer = 0.f;
	float NextAttackInterval = 0.f;

	// Angle offset rotates entire circle over time
	float CircleOffset = 0.f;

	float CircleTimer = 0.f;
	float HaltTimer = 0.f;
	float WindupTimer = 0.f;
	float RecoveryTimer = 0.f;
};
