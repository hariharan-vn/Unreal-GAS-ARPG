// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "WeaponActor.generated.h"

class UNiagaraComponent;
class UStaticMeshComponent;
class UNiagaraSystem;

UCLASS()
class GAS_ARPG_API AWeaponActor : public AActor
{
	GENERATED_BODY()
	
public:
	AWeaponActor();

	// Trail/swing VFX — triggered by ability
	void PlaySwingEffect() const;

	// Optional hit spark at impact point
	void PlayHitEffect(const FVector& HitLocation) const;
	
	void StopSwingEffect() const;

	FVector GetSocketLocation(const FName& SocketName) const;

	bool HasSocket(const FName& SocketName) const;
	
	
private:
	// The weapon mesh
	UPROPERTY(VisibleAnywhere, Category = "Components")
	TObjectPtr<UStaticMeshComponent> WeaponMesh;

	// Optional swing trail VFX
	UPROPERTY(VisibleAnywhere, Category = "Components")
	TObjectPtr<UNiagaraComponent> SwingTrailEffect;

	// Configured in editor per weapon type
	UPROPERTY(EditDefaultsOnly, Category = "Effects")
	TObjectPtr<UNiagaraSystem> HitVFX;
};
