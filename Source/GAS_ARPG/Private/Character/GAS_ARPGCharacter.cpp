// Copyright Epic Games, Inc. All Rights Reserved.

#include "Character/GAS_ARPGCharacter.h"

#include "AbilitySystemBlueprintLibrary.h"
#include "EnhancedInputComponent.h"
#include "UObject/ConstructorHelpers.h"
#include "Camera/CameraComponent.h"

#include "GameFramework/PlayerController.h"
#include "GameFramework/SpringArmComponent.h"
#include "Materials/Material.h"
#include "Engine/World.h"
#include "Items/Weapon/WeaponPickup.h"
#include "Items/Weapon/WeaponPickupPayload.h"
#include "Player/RPGPlayerState.h"

AGAS_ARPGCharacter::AGAS_ARPGCharacter()
{
	// Create a camera boom...
	CameraBoom = CreateDefaultSubobject<USpringArmComponent>(TEXT("CameraBoom"));
	CameraBoom->SetupAttachment(RootComponent);
	CameraBoom->SetUsingAbsoluteRotation(true); // Don't want arm to rotate when character does
	CameraBoom->TargetArmLength = 800.f;
	CameraBoom->SetRelativeRotation(FRotator(-60.f, 0.f, 0.f));
	CameraBoom->bDoCollisionTest = false; // Don't want to pull camera in when it collides with level

	// Create a camera...
	TopDownCameraComponent = CreateDefaultSubobject<UCameraComponent>(TEXT("TopDownCamera"));
	TopDownCameraComponent->SetupAttachment(CameraBoom, USpringArmComponent::SocketName);
	TopDownCameraComponent->bUsePawnControlRotation = false; // Camera does not rotate relative to arm

	// Activate ticking in order to update the cursor every frame.
	PrimaryActorTick.bCanEverTick = true;
	PrimaryActorTick.bStartWithTickEnabled = true;
}

void AGAS_ARPGCharacter::PossessedBy(AController* NewController)
{
	Super::PossessedBy(NewController);

	if (ARPGPlayerState* OwningPlayerState = GetPlayerState<ARPGPlayerState>())
	{
		OwningPlayerState->PushASCToPawn();
	}
}

void AGAS_ARPGCharacter::OnRep_PlayerState()
{
	Super::OnRep_PlayerState();

	if (ARPGPlayerState* OwningPlayerState = GetPlayerState<ARPGPlayerState>())
	{
		OwningPlayerState->PushASCToPawn();
	}
}

void AGAS_ARPGCharacter::SetAbilitySystemComponent(UAbilitySystemComponent* InAbilitySystemComponent)
{
	CachedAbilitySystemComponent = InAbilitySystemComponent;
}

void AGAS_ARPGCharacter::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);

	UEnhancedInputComponent* EIC = Cast<UEnhancedInputComponent>(PlayerInputComponent);
	if (!EIC) return;

	if (IA_Move)
	{
		EIC->BindAction(IA_Move, ETriggerEvent::Triggered, this, &AGAS_ARPGCharacter::Input_Move);
	}

	if (IA_Look)
	{
		EIC->BindAction(IA_Look, ETriggerEvent::Triggered, this, &AGAS_ARPGCharacter::Input_Look);
	}

	if (IA_LeapSlam)
	{
		EIC->BindAction(IA_LeapSlam, ETriggerEvent::Started, this, &AGAS_ARPGCharacter::Input_LeapSlam);
	}

	if (IA_BasicAttack)
	{
		EIC->BindAction(IA_BasicAttack, ETriggerEvent::Started, this, &AGAS_ARPGCharacter::Input_BasicAttack);
	}

	if (IA_Interact)
	{
		EIC->BindAction(IA_Interact, ETriggerEvent::Started, this, &AGAS_ARPGCharacter::Input_Interact);
	}
}

UAbilitySystemComponent* AGAS_ARPGCharacter::GetAbilitySystemComponent() const
{
	return CachedAbilitySystemComponent.IsValid() ? CachedAbilitySystemComponent.Get() : nullptr;
}

void AGAS_ARPGCharacter::SetNearbyWeapon(AWeaponPickup* Pickup)
{
	NearbyWeapon = Pickup;
}

AWeaponPickup* AGAS_ARPGCharacter::GetNearbyWeapon() const
{
	return NearbyWeapon.IsValid() ? NearbyWeapon.Get() : nullptr;
}

void AGAS_ARPGCharacter::Input_Move(const FInputActionValue& Value)
{
	const FVector2D Axis = Value.Get<FVector2D>();
	const FRotator Yaw(0.f, GetControlRotation().Yaw, 0.f);

	// Forward/back
	AddMovementInput(FRotationMatrix(Yaw).GetUnitAxis(EAxis::X), Axis.Y);
	// Left/right strafe
	AddMovementInput(FRotationMatrix(Yaw).GetUnitAxis(EAxis::Y), Axis.X);
}

void AGAS_ARPGCharacter::Input_Look(const FInputActionValue& Value)
{
	const FVector2D Axis = Value.Get<FVector2D>();
	AddControllerYawInput(Axis.X);
	AddControllerPitchInput(Axis.Y);
}

void AGAS_ARPGCharacter::Input_LeapSlam(const FInputActionValue& Value)
{
	if (!GetAbilitySystemComponent()) return;

	UAbilitySystemBlueprintLibrary::SendGameplayEventToActor(this, LeapSlamTag, FGameplayEventData());
}

void AGAS_ARPGCharacter::Input_BasicAttack(const FInputActionValue& Value)
{
	if (!GetAbilitySystemComponent()) return;

	UAbilitySystemBlueprintLibrary::SendGameplayEventToActor(this, BasicAttackTag, FGameplayEventData());
}

void AGAS_ARPGCharacter::Input_Interact(const FInputActionValue& Value)
{
	if (!NearbyWeapon.IsValid()) return;

	UAbilitySystemComponent* ASC = GetAbilitySystemComponent();
	if (!ASC) return;

	UWeaponPickupPayload* Payload = UWeaponPickupPayload::Create(this, NearbyWeapon->GetWeaponData());

	// Pass pickup actor through EventData
	FGameplayEventData EventData;
	EventData.OptionalObject = Payload;

	UAbilitySystemBlueprintLibrary::SendGameplayEventToActor(this, WeaponPickupTag, EventData);

	// Clear reference — weapon is being picked up
	NearbyWeapon->Destroy();
	NearbyWeapon = nullptr;
}

void AGAS_ARPGCharacter::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
}
