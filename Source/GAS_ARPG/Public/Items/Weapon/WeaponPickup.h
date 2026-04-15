// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "WeaponData.h"
#include "GameFramework/Actor.h"
#include "WeaponPickup.generated.h"

class USphereComponent;
class UStaticMeshComponent;

UCLASS()
class GAS_ARPG_API AWeaponPickup : public AActor
{
	GENERATED_BODY()

public:
	AWeaponPickup();

	const FWeaponData& GetWeaponData() const { return WeaponData; }
	
	void OnPickedUp();
	
protected:
	virtual void BeginPlay() override;

private:
	UFUNCTION()
	void OnSphereBeginOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor,
	                          UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep,
	                          const FHitResult& SweepResult);

	UFUNCTION()
	void OnSphereEndOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor,
	                        UPrimitiveComponent* OtherComp, int32 OtherBodyIndex);

	// Weapon this pickup represents
	// Set per instance in editor via EditInstanceOnly
	UPROPERTY(EditAnywhere, Category = "Weapon")
	FWeaponData WeaponData;

	UPROPERTY(VisibleAnywhere, Category = "Components")
	TObjectPtr<USphereComponent> OverlapSphere;

	UPROPERTY(VisibleAnywhere, Category = "Components")
	TObjectPtr<UStaticMeshComponent> WeaponMesh;
};
