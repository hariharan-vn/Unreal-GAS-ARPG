#include "GameplayAbilitySystem/Tasks/AbilityTask_LeapMovement.h"

#include "GameFramework/Character.h"

UAbilityTask_LeapMovement* UAbilityTask_LeapMovement::CreateLeapMovementTask(
	UGameplayAbility* OwningAbility,
	const FVector InTargetLocation,
	const float InLeapDuration,
	const float InArcHeight)
{
	UAbilityTask_LeapMovement* Task = NewAbilityTask<UAbilityTask_LeapMovement>(OwningAbility);
	Task->TargetLocation = InTargetLocation;
	Task->TotalLeapDuration = InLeapDuration;
	Task->ArcHeight = InArcHeight;
	Task->ElapsedTime = 0.f;
	return Task;
}

void UAbilityTask_LeapMovement::Activate()
{
	Super::Activate();

	OwnerCharacter = Cast<ACharacter>(GetAvatarActor());
	if (!OwnerCharacter.IsValid())
	{
		OnLeapFailed.Broadcast();
		EndTask();
		return;
	}

	StartLocation = OwnerCharacter->GetActorLocation();

	// Must set this to receive TickTask() calls
	bTickingTask = true;
}

void UAbilityTask_LeapMovement::TickTask(float DeltaTime)
{
	Super::TickTask(DeltaTime);

	if (!OwnerCharacter.IsValid())
	{
		EndTask();
		return;
	}

	ElapsedTime += DeltaTime;
	const float Alpha = FMath::Clamp(ElapsedTime / TotalLeapDuration, 0.f, 1.f);

	// Lerp XY + parabolic Z arc
	FVector NewPos = FMath::Lerp(StartLocation, TargetLocation, Alpha);
	NewPos.Z = FMath::Lerp(StartLocation.Z, TargetLocation.Z, Alpha)
		+ ArcHeight * FMath::Sin(Alpha * PI);

	FHitResult Hit;
	OwnerCharacter->SetActorLocation(NewPos, true, &Hit);

	// Blocked mid-air (hit a wall etc.)
	if (Hit.bBlockingHit && Alpha < 1.f)
	{
		OnLeapFailed.Broadcast();
		EndTask();
		return;
	}

	if (Alpha >= 1.f)
	{
		OwnerCharacter->SetActorLocation(TargetLocation);
		OnLeapCompleted.Broadcast();
		EndTask();
	}
}

void UAbilityTask_LeapMovement::OnDestroy(bool bInOwnerFinished)
{
	// Cleanup if ability was cancelled mid-leap
	Super::OnDestroy(bInOwnerFinished);
}
