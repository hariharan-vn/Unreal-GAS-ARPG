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
#include "GAS_ARPG/GAS_ARPG.h"
#include "Items/Weapon/WeaponActor.h"
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

	if (MoveAction)
	{
		EIC->BindAction(MoveAction, ETriggerEvent::Triggered, this, &AGAS_ARPGCharacter::Input_Move);
	}

	if (EquipAction)
	{
		EIC->BindAction(EquipAction, ETriggerEvent::Started, this, &AGAS_ARPGCharacter::Input_Equip);
	}

	if (AttackAction)
	{
		EIC->BindAction(AttackAction, ETriggerEvent::Started, this, &AGAS_ARPGCharacter::Input_Attack);
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

void AGAS_ARPGCharacter::InitializePawnASC(AActor* ASCOwner)
{
	Super::InitializePawnASC(ASCOwner);

	if (CachedAbilitySystemComponent.IsValid())
	{
		CachedAbilitySystemComponent->GenericGameplayEventCallbacks.FindOrAdd(WeaponEquipEventTag).AddUObject(
			this, &AGAS_ARPGCharacter::OnWeaponEquipped);
	}
}

void AGAS_ARPGCharacter::Input_Move(const FInputActionValue& Value)
{
	if (const UAbilitySystemComponent* ASC = GetAbilitySystemComponent())
	{
		if (ASC->HasMatchingGameplayTag(AttackingTag))
		{
			return;
		}
	}

	const FVector2D Axis = Value.Get<FVector2D>();
	const FRotator Yaw(0.f, GetControlRotation().Yaw, 0.f);

	// Forward/back
	AddMovementInput(FRotationMatrix(Yaw).GetUnitAxis(EAxis::X), Axis.Y);
	// Left/right strafe
	AddMovementInput(FRotationMatrix(Yaw).GetUnitAxis(EAxis::Y), Axis.X);
}

bool AGAS_ARPGCharacter::CanSwitchAbility() const
{
	if (const UAbilitySystemComponent* ASC = GetAbilitySystemComponent())
	{
		FGameplayTagContainer GameplayTags;
		ASC->GetOwnedGameplayTags(GameplayTags);
		for (auto Tag : GameplayTags)
		{
			UE_LOG(LogTemp, Warning, TEXT("CanSwitch %s"), *Tag.ToString());
		}
		UE_LOG(LogTemp, Warning, TEXT("================== %d"), GameplayTags.Num());

		return !ASC->HasMatchingGameplayTag(AttackingTag);
	}
	return false;
}

void AGAS_ARPGCharacter::Input_SwapWeapon(const FInputActionValue& Value)
{
}

void AGAS_ARPGCharacter::Input_Attack(const FInputActionValue& Value)
{
	if (!WeaponAbilityTag.IsValid())
	{
		return;
	}
	UE_LOG(LogTemp, Warning, TEXT("Input_Attack"));

	if (!GetAbilitySystemComponent()) return;

	UAbilitySystemBlueprintLibrary::SendGameplayEventToActor(this, WeaponAbilityTag, FGameplayEventData());
}

void AGAS_ARPGCharacter::Input_Equip(const FInputActionValue& Value)
{
	if (!NearbyWeapon.IsValid()) return;

	if (!GetAbilitySystemComponent()) return;

	UWeaponPickupPayload* Payload = UWeaponPickupPayload::Create(this, NearbyWeapon->GetWeaponData());

	// Pass pickup actor through EventData
	FGameplayEventData EventData;
	EventData.OptionalObject = Payload;

	UAbilitySystemBlueprintLibrary::SendGameplayEventToActor(this, PickupEventTag, EventData);

	// Clear reference — weapon is being picked up
	NearbyWeapon->Destroy();
	NearbyWeapon = nullptr;
}

void AGAS_ARPGCharacter::OnWeaponEquipped(const FGameplayEventData* GameplayEventData)
{
	if (const auto EquippedWeapon = Cast<AWeaponActor>(GameplayEventData->OptionalObject))
	{
		WeaponAbilityTag = EquippedWeapon->GetWeaponAbilityTag();
		GetMesh()->SetAnimInstanceClass(EquippedWeapon->GetAnimBPClass());
	}
}

void AGAS_ARPGCharacter::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
}
