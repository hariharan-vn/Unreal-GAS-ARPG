#include "AI/EnemyGroupManager.h"

#include "AIController.h"
#include "Character/EnemyCharacter.h"
#include "GAS_ARPG/GAS_ARPG.h"
#include "Perception/AIPerceptionComponent.h"
#include "Perception/AISense_Sight.h"

AEnemyGroupManager::AEnemyGroupManager()
{
	PrimaryActorTick.bCanEverTick = true;
}

// Called when the game starts or when spawned
void AEnemyGroupManager::BeginPlay()
{
	Super::BeginPlay();

	for (auto Enemy : RegisteredEnemies)
	{
		if (Enemy)
		{
			Enemy->GroupManager = this;
		}
	}
	NextAttackInterval = FMath::RandRange(MinAttackInterval, MaxAttackInterval);
}

void AEnemyGroupManager::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	if (!TrackedPlayer.IsValid()) return;

	HandleState(DeltaTime);
}

void AEnemyGroupManager::SetGroupState(EGroupState NewState)
{
	if (CurrentState == NewState) return;

	CurrentState = NewState;

	UE_LOG(ARPG_Ability, Verbose, TEXT("[%hs] Group state → %d"), __FUNCTION__, (uint8)NewState);

	switch (NewState)
	{
	case EGroupState::Idle:
		TrackedPlayer = nullptr;
		CurrentAttacker = nullptr;
		StopAllEnemies();
		break;

	case EGroupState::Circling:
		CircleTimer = 0.f;
		UpdateCirclePositions();
		break;

	case EGroupState::Halting:
		// Stop all enemies in place
		StopAllEnemies();
		HaltTimer = 0.f;
		break;

	case EGroupState::AttackWindup:
		// Attacker already selected — just wait
		WindupTimer = 0.f;
		break;

	case EGroupState::Attacking:
		// Start moving attacker in
		if (CurrentAttacker.IsValid() && TrackedPlayer.IsValid())
		{
			CurrentAttacker->MoveInToAttack(
				TrackedPlayer->GetActorLocation());
		}
		break;

	case EGroupState::AttackRecovery:
		CurrentAttacker = nullptr;
		RecoveryTimer = 0.f;
		break;

	default:
		break;
	}
}

void AEnemyGroupManager::OnEnemyDetectedPlayer(AEnemyCharacter* Enemy, AActor* Player)
{
	TrackedPlayer = Player;

	if (CurrentState == EGroupState::Idle || CurrentState == EGroupState::Investigating)
	{
		if (IsPlayerWithinEngageRadius())
		{
			SetGroupState(EGroupState::Circling);
		}
		else
		{
			SetGroupState(EGroupState::Investigating);
			// Move all enemies toward last known position
			for (AEnemyCharacter* E : GetAliveEnemies())
			{
				E->MoveToLocation(Player->GetActorLocation());
			}
		}
	}
}

void AEnemyGroupManager::OnEnemyLostPlayer(const AEnemyCharacter* Enemy, AActor* Player)
{
	// Only go idle if ALL enemies lost sight
	for (AEnemyCharacter* E : GetAliveEnemies())
	{
		if (E == Enemy) continue; // skip the one that just lost sight

		// Check if this enemy currently has the player in perception
		TArray<AActor*> PerceivedActors;
		E->GetAIPerceptionComponent()->GetCurrentlyPerceivedActors(UAISense_Sight::StaticClass(), PerceivedActors);

		if (PerceivedActors.Contains(Player))
		{
			return; // at least one enemy still sees player
		}
	}

	SetGroupState(EGroupState::Investigating);
}

void AEnemyGroupManager::UpdateCirclePositions()
{
	if (!TrackedPlayer.IsValid()) return;

	TArray<AEnemyCharacter*> Alive = GetAliveEnemies();
	if (Alive.IsEmpty()) return;

	const FVector PlayerPos = TrackedPlayer->GetActorLocation();
	const float AngleStep = 360.f / Alive.Num();

	for (int32 i = 0; i < Alive.Num(); i++)
	{
		AEnemyCharacter* Enemy = Alive[i];

		// Skip current attacker — they're moving in
		if (Enemy == CurrentAttacker) continue;

		const float Angle = FMath::DegreesToRadians(i * AngleStep + CircleOffset);

		const FVector SlotPosition = PlayerPos + FVector(FMath::Cos(Angle) * CircleRadius,
		                                                 FMath::Sin(Angle) * CircleRadius, 0.f);

		// Only update if slot has moved significantly
		if (FVector::Dist2D(Enemy->AssignedSlotPosition, SlotPosition) > 50.f)
		{
			Enemy->FallBackToSlot(SlotPosition);
		}
	}
}


void AEnemyGroupManager::UpdateAttacking(const float DeltaTime)
{
	if (!CurrentAttacker.IsValid() || !TrackedPlayer.IsValid())
	{
		SetGroupState(EGroupState::AttackRecovery);
		return;
	}

	// Keep updating target as player moves — throttled
	CurrentAttacker->MoveInToAttack(TrackedPlayer->GetActorLocation());

	const float DistToPlayer = FVector::Dist2D(CurrentAttacker->GetActorLocation(), TrackedPlayer->GetActorLocation());

	if (DistToPlayer <= AttackRange)
	{
		UE_LOG(ARPG_Ability, Verbose, TEXT("[%hs] %s reached attack range — recovering"), __FUNCTION__,
		       *CurrentAttacker->GetName());

		// Fall back to slot
		CurrentAttacker->FallBackToSlot(CurrentAttacker->AssignedSlotPosition);

		SetGroupState(EGroupState::AttackRecovery);
	}
}

void AEnemyGroupManager::SelectNextAttacker()
{
	TArray<AEnemyCharacter*> Alive = GetAliveEnemies();
	if (Alive.IsEmpty()) return;

	// Pick random enemy from alive group
	const int32 Index = FMath::RandRange(0, Alive.Num() - 1);
	CurrentAttacker = Alive[Index];

	// Move attacker toward player
	if (TrackedPlayer.IsValid())
	{
		CurrentAttacker->MoveInToAttack(TrackedPlayer->GetActorLocation());
	}
}

bool AEnemyGroupManager::IsPlayerWithinEngageRadius() const
{
	if (!TrackedPlayer.IsValid()) return false;
	return FVector::Dist2D(GetActorLocation(), TrackedPlayer->GetActorLocation()) <= EngageRadius;
}

bool AEnemyGroupManager::IsPlayerBeyondBreakRadius() const
{
	if (!TrackedPlayer.IsValid()) return true;
	return FVector::Dist2D(GetActorLocation(), TrackedPlayer->GetActorLocation()) > BreakRadius;
}

TArray<AEnemyCharacter*> AEnemyGroupManager::GetAliveEnemies() const
{
	TArray<AEnemyCharacter*> Alive;
	for (AEnemyCharacter* Enemy : RegisteredEnemies)
	{
		if (Enemy && IsValid(Enemy))
		{
			Alive.Add(Enemy);
		}
	}
	return Alive;
}

void AEnemyGroupManager::HandleState(const float DeltaTime)
{
	// Rotate circle offset only while actively circling
	if (CurrentState == EGroupState::Circling)
	{
		CircleOffset += DeltaTime * 15.f;
		if (CircleOffset >= 360.f) CircleOffset -= 360.f;
	}

	switch (CurrentState)
	{
	case EGroupState::Circling:
		UpdateCirclePositions();
		CircleTimer += DeltaTime;
		if (CircleTimer >= CircleDuration)
		{
			CircleTimer = 0.f;
			SetGroupState(EGroupState::Halting);
		}
		break;

	case EGroupState::Halting:
		HaltTimer += DeltaTime;
		if (HaltTimer >= HaltDuration)
		{
			HaltTimer = 0.f;
			SelectNextAttacker();
			SetGroupState(EGroupState::AttackWindup);
		}
		break;

	case EGroupState::AttackWindup:
		WindupTimer += DeltaTime;
		if (WindupTimer >= AttackWindupDuration)
		{
			WindupTimer = 0.f;
			SetGroupState(EGroupState::Attacking);
		}
		break;

	case EGroupState::Attacking:
		UpdateAttacking(DeltaTime);
		break;

	case EGroupState::AttackRecovery:
		RecoveryTimer += DeltaTime;
		if (RecoveryTimer >= AttackRecoveryDuration)
		{
			RecoveryTimer = 0.f;
			SetGroupState(EGroupState::Circling);
		}
		break;

	default:
		break;
	}
}

void AEnemyGroupManager::OnEnemyDied(const AEnemyCharacter* Enemy)
{
	// If attacker died, reset
	if (CurrentAttacker == Enemy)
	{
		CurrentAttacker = nullptr;
		AttackTimer = 0.f;
	}

	// Recalculate slots with remaining enemies
	UpdateCirclePositions();
}

void AEnemyGroupManager::StopAllEnemies() const
{
	for (const AEnemyCharacter* Enemy : GetAliveEnemies())
	{
		if (AAIController* AIC = Cast<AAIController>(Enemy->GetController()))
		{
			AIC->StopMovement();
		}
	}
}
