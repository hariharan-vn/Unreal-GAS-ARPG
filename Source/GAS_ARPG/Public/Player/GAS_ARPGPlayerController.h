// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Templates/SubclassOf.h"
#include "GameFramework/PlayerController.h"
#include "GAS_ARPGPlayerController.generated.h"

DECLARE_LOG_CATEGORY_EXTERN(LogTemplateCharacter, Log, All);

class UInputMappingContext;

UCLASS()
class AGAS_ARPGPlayerController : public APlayerController
{
	GENERATED_BODY()

public:
	AGAS_ARPGPlayerController();

	/** MappingContext */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category=Input, meta=(AllowPrivateAccess = "true"))
	UInputMappingContext* DefaultMappingContext;

protected:
	virtual void SetupInputComponent() override;
};
