#include "Character/EnemyCharacter.h"

#include "AIController.h"
#include "AI/EnemyGroupManager.h"
#include "GameplayAbilitySystem/AttributeSets/BasicAttributeSet.h"
#include "GAS_ARPG/GAS_ARPG.h"
#include "Perception/AIPerceptionComponent.h"
#include "Perception/AISenseConfig_Sight.h"

AEnemyCharacter::AEnemyCharacter()
{
	PrimaryActorTick.bCanEverTick = true;

	AbilitySystemComponent = CreateDefaultSubobject<UAbilitySystemComponent>(TEXT("AbilitySystemComponent"));
	AbilitySystemComponent->SetIsReplicated(true);
	AbilitySystemComponent->SetReplicationMode(EGameplayEffectReplicationMode::Minimal);

	BasicAttributeSet = CreateDefaultSubobject<UBasicAttributeSet>("BasicAttributeSet");

	PerceptionComponent = CreateDefaultSubobject<UAIPerceptionComponent>(TEXT("AIPerception"));

	SightConfig = CreateDefaultSubobject<UAISenseConfig_Sight>(TEXT("SightConfig"));

	SightConfig->SightRadius = 1500.f;
	SightConfig->LoseSightRadius = 2000.f;
	SightConfig->PeripheralVisionAngleDegrees = 60.f; // 60 degrees = 120 degree cone (coverage)
	SightConfig->SetMaxAge(5.f);
	SightConfig->DetectionByAffiliation.bDetectEnemies = true;
	SightConfig->DetectionByAffiliation.bDetectNeutrals = true;
	SightConfig->DetectionByAffiliation.bDetectFriendlies = true;

	PerceptionComponent->ConfigureSense(*SightConfig);
	PerceptionComponent->SetDominantSense(SightConfig->GetSenseImplementation());
}

UAIPerceptionComponent* AEnemyCharacter::GetAIPerceptionComponent() const
{
	return PerceptionComponent;
}

void AEnemyCharacter::BeginPlay()
{
	Super::BeginPlay();

	PerceptionComponent->OnTargetPerceptionUpdated.AddDynamic(this, &AEnemyCharacter::OnTargetPerceptionUpdated);
}


void AEnemyCharacter::OnTargetPerceptionUpdated(AActor* Actor, FAIStimulus Stimulus)
{
	if (!GroupManager.IsValid()) return;

	if (GetTeamFromActor(Actor) != ETeam::Player) return;

	if (Stimulus.WasSuccessfullySensed())
	{
		GroupManager->OnEnemyDetectedPlayer(this, Actor);
	}
	else
	{
		GroupManager->OnEnemyLostPlayer(this, Actor);
	}
}

void AEnemyCharacter::MoveToLocation(const FVector& Destination) const
{
	if (AAIController* AIC = Cast<AAIController>(GetController()))
	{
		AIC->MoveToLocation(Destination, 50.f); // 50cm acceptance radius
	}
}

void AEnemyCharacter::MoveInToAttack(const FVector& AttackPosition)
{
	UE_LOG(ARPG_Ability, Verbose, TEXT("[%hs] %s moving in to attack"), __FUNCTION__, *GetName());

	MoveUpdateThrottle += GetWorld()->GetDeltaSeconds();
	if (MoveUpdateThrottle < 0.2f) return; // update every 0.2s
	MoveUpdateThrottle = 0.f;
	
	MoveToLocation(AttackPosition);
}

void AEnemyCharacter::FallBackToSlot(const FVector& SlotPosition)
{
	UE_LOG(ARPG_Ability, Verbose, TEXT("[%hs] %s falling back to slot"), __FUNCTION__, *GetName());

	AssignedSlotPosition = SlotPosition;
	MoveToLocation(SlotPosition);
}


void AEnemyCharacter::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	if (!GroupManager.IsValid()) return;
	if (!GroupManager->GetTrackedPlayer().IsValid()) return;

	// Always face the player
	const FVector Direction = (GroupManager->GetTrackedPlayer()->GetActorLocation() - GetActorLocation()).
		GetSafeNormal2D();

	if (!Direction.IsNearlyZero())
	{
		const FRotator TargetRotation = Direction.Rotation();
		const FRotator CurrentRotation = GetActorRotation();
		SetActorRotation(FMath::RInterpTo(CurrentRotation, TargetRotation, DeltaTime, 10.f));
	}
}

void AEnemyCharacter::PossessedBy(AController* NewController)
{
	Super::PossessedBy(NewController);

	CachedAbilitySystemComponent = AbilitySystemComponent;

	InitializePawnASC(this);
}

UAbilitySystemComponent* AEnemyCharacter::GetAbilitySystemComponent() const
{
	return AbilitySystemComponent;
}
