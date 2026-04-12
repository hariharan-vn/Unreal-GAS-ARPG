#include "Character/ARPGCharacterBase.h"
#include "Components/CapsuleComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameplayAbilitySystem/AttributeSets/BasicAttributeSet.h"

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
	if (UAbilitySystemComponent* ASC = GetAbilitySystemComponent())
	{
		ASC->InitAbilityActorInfo(ASCOwner, this);
	}
	InitializeAttributes();
	GiveDefaultAbilities();

	if (UBasicAttributeSet* AttribSet = const_cast<UBasicAttributeSet*>(GetAbilitySystemComponent()->GetSet<
		UBasicAttributeSet>()))
	{
		AttribSet->OnDeath.AddUObject(this, &AARPGCharacterBase::HandleDeath);
	}
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

	// TODO: Play death montage
	// TODO: Enable ragdoll
	// TODO: Disable collision

	UE_LOG(LogTemp, Warning, TEXT("[ARPGCharacterBase] %s HandleDeath"),
	       *GetName());
}
