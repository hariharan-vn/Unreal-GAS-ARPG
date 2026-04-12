// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "WeaponData.h"
#include "UObject/Object.h"
#include "WeaponPickupPayload.generated.h"

UCLASS()
class GAS_ARPG_API UWeaponPickupPayload : public UObject
{
	GENERATED_BODY()

public:
	static UWeaponPickupPayload* Create(UObject* Outer, const FWeaponData& InWeaponData)
	{
		UWeaponPickupPayload* Payload =
			NewObject<UWeaponPickupPayload>(Outer);
		Payload->WeaponData = InWeaponData;
		return Payload;
	}

	const FWeaponData& GetWeaponData() const { return WeaponData; }

private:
	UPROPERTY()
	FWeaponData WeaponData;
};
