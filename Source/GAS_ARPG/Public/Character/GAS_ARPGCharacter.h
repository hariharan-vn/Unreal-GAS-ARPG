// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "ARPGCharacterBase.h"
#include "GAS_ARPGCharacter.generated.h"

class AWeaponActor;
struct FWeaponAbilityConfig;
struct FWeaponData;
struct FInputActionValue;
class UCameraComponent;
class USpringArmComponent;
class UAbilitySystemComponent;
class UInputComponent;
class UInputAction;
class AWeaponPickup;
class UGameplayAbility;


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

	virtual void InitializePawnASC(AActor* ASCOwner) override;

private:
	bool CanSwitchAbility() const;

	void Input_Move(const FInputActionValue& Value);

	void Input_SwapWeapon(const FInputActionValue& Value);

	void Input_Attack(const FInputActionValue& Value);

	void Input_Equip(const FInputActionValue& Value);

	void OnWeaponEquipped(const FGameplayEventData* GameplayEventData);

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = Camera, meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UCameraComponent> TopDownCameraComponent;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = Camera, meta = (AllowPrivateAccess = "true"))
	TObjectPtr<USpringArmComponent> CameraBoom;

	TWeakObjectPtr<AWeaponPickup> NearbyWeapon;

	UPROPERTY(EditDefaultsOnly, Category = "Input")
	TObjectPtr<UInputAction> MoveAction;

	UPROPERTY(EditDefaultsOnly, Category = "Input")
	TObjectPtr<UInputAction> EquipAction;

	UPROPERTY(EditDefaultsOnly, Category = "Input")
	TObjectPtr<UInputAction> AttackAction;

	UPROPERTY(EditDefaultsOnly, Category = "Ability")
	FGameplayTag WeaponAbilityTag;
	
	UPROPERTY(EditDefaultsOnly, Category = "Ability")
	FGameplayTag AttackingTag;

	UPROPERTY(EditDefaultsOnly, Category = "Input")
	FGameplayTag PickupEventTag;

	UPROPERTY(EditDefaultsOnly, Category = "Input")
	FGameplayTag WeaponEquipEventTag;

	int32 CurrentActiveSlotIndex = -1;
};
