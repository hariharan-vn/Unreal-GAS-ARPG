#pragma once
#include "GameplayTagContainer.h"

#include "WeaponData.generated.h"

class UGameplayEffect;
class AActor;

USTRUCT(BlueprintType)
struct FWeaponData
{
	GENERATED_BODY()

	// Display name for UI
	UPROPERTY(EditDefaultsOnly)
	FText WeaponName;

	UPROPERTY(EditDefaultsOnly)
	FName SocketName = "WeaponSocket";

	UPROPERTY(EditDefaultsOnly)
	TSubclassOf<AActor> WeaponActorClass;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly)
	FGameplayTag WeaponTypeTag;

	// W — weapon attack speed after local modifiers
	// This feeds directly into LeapSlam duration formula
	UPROPERTY(EditDefaultsOnly)
	float WeaponAttackSpeed = 1.0f;
};
