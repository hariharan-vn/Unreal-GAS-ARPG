// Fill out your copyright notice in the Description page of Project Settings.

#include "Items/Weapon/WeaponActor.h"
#include "NiagaraComponent.h"
#include "NiagaraFunctionLibrary.h"

AWeaponActor::AWeaponActor()
{
	PrimaryActorTick.bCanEverTick = false;

	WeaponMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("WeaponMesh"));
	SetRootComponent(WeaponMesh);

	WeaponMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);

	SwingTrailEffect = CreateDefaultSubobject<UNiagaraComponent>(TEXT("SwingTrail"));
	SwingTrailEffect->SetupAttachment(RootComponent);
	SwingTrailEffect->SetAutoActivate(false); // activated by ability
}

void AWeaponActor::PlaySwingEffect() const
{
	if (SwingTrailEffect)
	{
		SwingTrailEffect->Activate(true);
	}
}

void AWeaponActor::PlayHitEffect(const FVector& HitLocation) const
{
	if (HitVFX)
	{
		UNiagaraFunctionLibrary::SpawnSystemAtLocation(
			GetWorld(),
			HitVFX,
			HitLocation
		);
	}
}

void AWeaponActor::StopSwingEffect() const
{
	if (SwingTrailEffect)
	{
		SwingTrailEffect->Deactivate();
	}
}

FVector AWeaponActor::GetSocketLocation(const FName& SocketName) const
{
	if (WeaponMesh)
	{
		return WeaponMesh->GetSocketLocation(SocketName);
	}
	UE_LOG(LogTemp, Warning, TEXT("[WeaponActor] Socket '%s' not found — using actor location"),
	       *SocketName.ToString());
	return GetActorLocation();
}

bool AWeaponActor::HasSocket(const FName& SocketName) const
{
	return WeaponMesh && WeaponMesh->DoesSocketExist(SocketName);
}

const FGameplayTag& AWeaponActor::GetWeaponAbilityTag() const
{
	return WeaponAbilityTag;
}
