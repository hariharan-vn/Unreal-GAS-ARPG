#include "Character/ARPGCharacterBase.h"
#include "Components/CapsuleComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameplayAbilitySystem/AttributeSets/BasicAttributeSet.h"
#include "GAS_ARPG/GAS_ARPG.h"

AARPGCharacterBase::AARPGCharacterBase()
{
	PrimaryActorTick.bCanEverTick = true;

	// Set size for player capsule
	GetCapsuleComponent()->InitCapsuleSize(42.f, 96.0f);

	// Don't rotate character to camera direction
	bUseControllerRotationPitch = false;
	bUseControllerRotationYaw = false;
	bUseControllerRotationRoll = false;

	// Configure character movement
	GetCharacterMovement()->bOrientRotationToMovement = true; // Rotate character to moving direction
	GetCharacterMovement()->RotationRate = FRotator(0.f, 640.f, 0.f);
	GetCharacterMovement()->bConstrainToPlane = true;
	GetCharacterMovement()->bSnapToPlaneAtStart = true;
}

void AARPGCharacterBase::InitializePawnASC(AActor* ASCOwner)
{
	UAbilitySystemComponent* ASC = GetAbilitySystemComponent();
	if (!ASC)
	{
		UE_LOG(ARPG_Ability, Warning, TEXT("[%hs] ASC not found"), __FUNCTION__);
		return;
	}

	ASC->InitAbilityActorInfo(ASCOwner, this);
	InitializeAttributes();
	GiveDefaultAbilities();

	UBasicAttributeSet* AttribSet = const_cast<UBasicAttributeSet*>(
		ASC->GetSet<UBasicAttributeSet>());
	if (!AttribSet)
	{
		UE_LOG(ARPG_Ability, Warning, TEXT("[%hs] AttributeSet not found"), __FUNCTION__);
		return;
	}

	AttribSet->OnDeath.AddUObject(this, &AARPGCharacterBase::HandleDeath);
}

void AARPGCharacterBase::SetLastHitDirection(const FVector& HitDirection)
{
	LastHitDirection = HitDirection;
}


void AARPGCharacterBase::InitializeAttributes()
{
	UAbilitySystemComponent* ASC = GetAbilitySystemComponent();
	if (!ASC || !GE_DefaultAttributes) return;

	FGameplayEffectContextHandle Context = ASC->MakeEffectContext();
	Context.AddSourceObject(this);

	const FGameplayEffectSpecHandle Spec = ASC->MakeOutgoingSpec(GE_DefaultAttributes, 1.f, Context);

	if (Spec.IsValid())
	{
		ASC->ApplyGameplayEffectSpecToSelf(*Spec.Data.Get());
	}
}

void AARPGCharacterBase::GiveDefaultAbilities()
{
	UAbilitySystemComponent* ASC = GetAbilitySystemComponent();
	if (!ASC || !HasAuthority()) return;

	for (const TSubclassOf<UGameplayAbility>& Ability : DefaultAbilities)
	{
		if (Ability)
		{
			ASC->GiveAbility(FGameplayAbilitySpec(Ability, 1));
		}
	}
}

void AARPGCharacterBase::HandleDeath(AActor* DeadActor)
{
	// Disable input
	if (APlayerController* PC = Cast<APlayerController>(GetController()))
	{
		PC->DisableInput(PC);
	}

	GetCapsuleComponent()->SetCollisionEnabled(ECollisionEnabled::NoCollision);

	// Disable movement
	GetCharacterMovement()->StopMovementImmediately();
	GetCharacterMovement()->DisableMovement();

	// Enable ragdoll on mesh
	GetMesh()->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
	GetMesh()->SetCollisionProfileName(TEXT("Ragdoll"));
	GetMesh()->SetAllBodiesSimulatePhysics(true);
	GetMesh()->SetSimulatePhysics(true);
	GetMesh()->WakeAllRigidBodies();
	GetMesh()->bBlendPhysics = true;

	if (LastHitDirection != FVector::ZeroVector)
	{
		GetMesh()->AddImpulse(LastHitDirection * DeathForce, NAME_None, true);
	}

	if (Controller)
	{
		Controller->SetIgnoreMoveInput(true);
		Controller->SetIgnoreLookInput(true);
	}

	SetLifeSpan(5.f);

	UE_LOG(LogTemp, Warning, TEXT("[%hs] %s Died"), __FUNCTION__, *GetName());
}
