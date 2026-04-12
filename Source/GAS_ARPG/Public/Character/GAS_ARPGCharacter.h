// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "ARPGCharacterBase.h"
#include "GameFramework/Character.h"
#include "GAS_ARPGCharacter.generated.h"

struct FInputActionValue;
class UCameraComponent;
class USpringArmComponent;
class UAbilitySystemComponent;
class UInputComponent;
class UInputAction;
class AWeaponPickup;


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
	void Input_Move(const FInputActionValue& Value);

	void Input_Look(const FInputActionValue& Value);

	void Input_LeapSlam(const FInputActionValue& Value);

	void Input_BasicAttack(const FInputActionValue& Value);

	void Input_Interact(const FInputActionValue& Value);


	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = Camera, meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UCameraComponent> TopDownCameraComponent;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = Camera, meta = (AllowPrivateAccess = "true"))
	TObjectPtr<USpringArmComponent> CameraBoom;

	TWeakObjectPtr<AWeaponPickup> NearbyWeapon;

	UPROPERTY(EditDefaultsOnly, Category = "Input")
	TObjectPtr<UInputAction> IA_Interact;

	UPROPERTY(EditDefaultsOnly, Category = "Input")
	TObjectPtr<UInputAction> IA_Move;

	UPROPERTY(EditDefaultsOnly, Category = "Input")
	TObjectPtr<UInputAction> IA_Look;

	UPROPERTY(EditDefaultsOnly, Category = "Input")
	TObjectPtr<UInputAction> IA_LeapSlam;

	UPROPERTY(EditDefaultsOnly, Category = "Input")
	TObjectPtr<UInputAction> IA_BasicAttack;

	UPROPERTY(EditDefaultsOnly, Category = "Ability")
	FGameplayTag LeapSlamTag;

	UPROPERTY(EditDefaultsOnly, Category = "Input")
	FGameplayTag BasicAttackTag;

	UPROPERTY(EditDefaultsOnly, Category = "Input")
	FGameplayTag WeaponPickupTag;
	
};
