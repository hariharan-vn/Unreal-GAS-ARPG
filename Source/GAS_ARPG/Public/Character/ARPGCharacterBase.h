#pragma once

#include "CoreMinimal.h"
#include "AbilitySystemComponent.h"
#include "GameFramework/Character.h"
#include "AbilitySystemInterface.h"
#include "GenericTeamAgentInterface.h"
#include "CommonEnum/CommonEnums.h"
#include "ARPGCharacterBase.generated.h"

class UInputComponent;
class UBasicAttributeSet;
class UGameplayAbility;
class UGameplayEffect;
class AActor;

UCLASS()
class GAS_ARPG_API AARPGCharacterBase : public ACharacter, public IAbilitySystemInterface,
                                        public IGenericTeamAgentInterface
{
	GENERATED_BODY()

public:
	AARPGCharacterBase();

	virtual UAbilitySystemComponent* GetAbilitySystemComponent() const override
	PURE_VIRTUAL(ARPGCharacterBase::GetAbilitySystemComponent, return nullptr;);

	virtual void InitializePawnASC(AActor* ASCOwner);

	void SetLastHitDirection(const FVector& HitDirection);

protected:
	void InitializeAttributes();

	void GiveDefaultAbilities();

	TWeakObjectPtr<UAbilitySystemComponent> CachedAbilitySystemComponent;

	UPROPERTY(EditDefaultsOnly, Category = "GAS")
	TArray<TSubclassOf<UGameplayAbility>> DefaultAbilities;

	UPROPERTY(EditDefaultsOnly, Category = "GAS")
	TSubclassOf<UGameplayEffect> GE_DefaultAttributes;

private:
	UFUNCTION()
	void HandleDeath(AActor* DeadActor);

	FVector LastHitDirection = FVector::ZeroVector;

	UPROPERTY(EditAnywhere, Category = "Death")
	float DeathForce = 5000.f;
};
