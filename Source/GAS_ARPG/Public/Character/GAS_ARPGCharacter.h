// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "ARPGCharacterBase.h"
#include "GameFramework/Character.h"
#include "GAS_ARPGCharacter.generated.h"

struct FWeaponData;
struct FInputActionValue;
class UCameraComponent;
class USpringArmComponent;
class UAbilitySystemComponent;
class UInputComponent;
class UInputAction;
class AWeaponPickup;

USTRUCT(BlueprintType)
struct FAbilitySlotConfig
{
	GENERATED_BODY()

	UPROPERTY(EditDefaultsOnly)
	TObjectPtr<UInputAction> InputAction;

	UPROPERTY(EditDefaultsOnly)
	FGameplayTag AbilityTag;

	UPROPERTY(EditDefaultsOnly)
	TSubclassOf<UAnimInstance> AnimBPClass;
};

UCLASS(Blueprintable)
class AGAS_ARPGCharacter : public AARPGCharacterBase
{
	GENERATED_BODY()

public:
	AGAS_ARPGCharacter();

	virtual void Tick(float DeltaSeconds) override;

	virtual void PossessedBy(AController* NewController) override;

	virtual void OnRep_PlayerState() override;

	FORCEINLINE UCameraComponent* GetTopDownCameraComponent() const
	{
		return TopDownCameraComponent;
	}

	FORCEINLINE USpringArmComponent* GetCameraBoom() const
	{
		return CameraBoom;
	}

	void SetAbilitySystemComponent(UAbilitySystemComponent* InAbilitySystemComponent);

	virtual void SetupPlayerInputComponent(UInputComponent* PlayerInputComponent) override;

	virtual UAbilitySystemComponent* GetAbilitySystemComponent() const override;

	void SetNearbyWeapon(AWeaponPickup* Pickup);

	AWeaponPickup* GetNearbyWeapon() const;

private:
	bool CanSwitchAbility() const;
	
	void Input_Move(const FInputActionValue& Value);

	void Input_SelectWeapon(const FInputActionValue& Value, const int32 SelectedIndex);

	void Input_Attack(const FInputActionValue& Value);

	void Input_Interact(const FInputActionValue& Value);

	void GrantDefaultWeapons();

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = Camera, meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UCameraComponent> TopDownCameraComponent;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = Camera, meta = (AllowPrivateAccess = "true"))
	TObjectPtr<USpringArmComponent> CameraBoom;

	UPROPERTY(EditDefaultsOnly, Category = "Ability Slots")
	TArray<FAbilitySlotConfig> AbilitySlots;

	UPROPERTY(EditAnywhere, Category = "Weapons")
	TArray<FWeaponData> DefaultWeapons;

	TWeakObjectPtr<AWeaponPickup> NearbyWeapon;

	UPROPERTY(EditDefaultsOnly, Category = "Input")
	TObjectPtr<UInputAction> InteractAction;

	UPROPERTY(EditDefaultsOnly, Category = "Input")
	TObjectPtr<UInputAction> MoveAction;

	UPROPERTY(EditDefaultsOnly, Category = "Input")
	TObjectPtr<UInputAction> AttackAction;

	UPROPERTY(EditDefaultsOnly, Category = "Ability")
	FGameplayTag AttackingTag;

	UPROPERTY(EditDefaultsOnly, Category = "Input")
	FGameplayTag EquipWeaponEventTag;

	FGameplayTag CurrentActiveAbilityTag = FGameplayTag::EmptyTag;
};
