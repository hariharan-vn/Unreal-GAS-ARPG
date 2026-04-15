#pragma once

#include "CoreMinimal.h"
#include "Animation/AnimInstance.h"
#include "ARPGCharacterAnimInstance.generated.h"


UENUM(BlueprintType)
enum class ECharacterAnimAbilityPhase : uint8
{
	None          UMETA(DisplayName = "None"),
	Anticipation  UMETA(DisplayName = "Anticipation"),  // startup / windup
	Active        UMETA(DisplayName = "Active"),         // the core action
	Recovery      UMETA(DisplayName = "Recovery")        // follow-through / land
};

UCLASS()
class GAS_ARPG_API UARPGCharacterAnimInstance : public UAnimInstance
{
	GENERATED_BODY()

	
};
