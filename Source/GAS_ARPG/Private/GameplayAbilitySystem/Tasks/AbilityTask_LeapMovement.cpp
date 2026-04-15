#include "GameplayAbilitySystem/Tasks/AbilityTask_LeapMovement.h"

#include "Components/CapsuleComponent.h"
#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "DrawDebugHelpers.h"

UAbilityTask_LeapMovement* UAbilityTask_LeapMovement::CreateLeapMovementTask(
	UGameplayAbility* OwningAbility, const FVector InTargetLocation, const float InLeapDuration,
	const float InArcHeight)
{
	UAbilityTask_LeapMovement* Task = NewAbilityTask<UAbilityTask_LeapMovement>(OwningAbility);
	Task->TargetLocation = InTargetLocation;
	Task->TotalLeapDuration = InLeapDuration;
	Task->ArcHeightRatio = InArcHeight;
	Task->ElapsedTime = 0.f;
	return Task;
}

void UAbilityTask_LeapMovement::Activate()
{
	Super::Activate();

	if (!EnsureCharacter())
	{
		OnLeapFailed.Broadcast();
		EndTask();
		return;
	}

	StartLocation = OwnerCharacter->GetActorLocation();
	const FVector TargetDirection = TargetLocation - StartLocation;
	const float Distance = (TargetDirection).Length();
	ArcHeight = Distance * ArcHeightRatio;

	if (!TargetDirection.IsNearlyZero())
	{
		OwnerCharacter->SetActorRotation(TargetDirection.GetSafeNormal2D().Rotation());
	}

	DrawDebugSphere(GetWorld(), StartLocation, 30, 10, FColor::Red, true);
	DrawDebugSphere(GetWorld(), TargetLocation, 30, 10, FColor::Blue, true);

	OwnerCharacter->GetCharacterMovement()->SetMovementMode(MOVE_Flying);
	OwnerCharacter->GetCharacterMovement()->StopMovementImmediately();

	OwnerCharacter->GetCapsuleComponent()->SetCollisionEnabled(ECollisionEnabled::NoCollision);

	CapsuleHalfHeight = OwnerCharacter->GetCapsuleComponent()->GetScaledCapsuleHalfHeight();

	ElapsedTime = 0.f;
	// Must set this to receive TickTask() calls
	bTickingTask = true;
}

void UAbilityTask_LeapMovement::TickTask(float DeltaTime)
{
	Super::TickTask(DeltaTime);

	if (!EnsureCharacter())
	{
		EndTask();
		return;
	}

	ElapsedTime += DeltaTime;
	const float Alpha = FMath::Clamp(ElapsedTime / TotalLeapDuration, 0.f, 1.f);

	// Lerp XY + parabolic Z arc
	FVector NewLocation = FMath::Lerp(StartLocation, TargetLocation, Alpha);
	NewLocation.Z += ArcHeight * FMath::Sin(Alpha * PI);

	NewLocation.Z += CapsuleHalfHeight;

	OwnerCharacter->SetActorLocation(NewLocation, false);

	OwnerCharacter->SetActorLocation(NewLocation, false);

	if (Alpha >= 1.f)
	{
		OwnerCharacter->SetActorLocation(TargetLocation);
		OnLeapCompleted.Broadcast();
		EndTask();
	}
}

void UAbilityTask_LeapMovement::OnDestroy(bool bInOwnerFinished)
{
	OnLeapCompleted.Clear();
	OnLeapFailed.Clear();

	if (EnsureCharacter())
	{
		OwnerCharacter->GetCapsuleComponent()->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);

		// Restore movement mode
		OwnerCharacter->GetCharacterMovement()->SetMovementMode(MOVE_Walking);
	}
	Super::OnDestroy(bInOwnerFinished);
}

bool UAbilityTask_LeapMovement::EnsureCharacter()
{
	if (OwnerCharacter.IsValid())
	{
		return true;
	}
	OwnerCharacter = Cast<ACharacter>(GetAvatarActor());
	return OwnerCharacter.IsValid();
}
