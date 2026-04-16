// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "AIController.h"
#include "CommonEnum/CommonEnums.h"
#include "ARPGAIController.generated.h"

UCLASS()
class GAS_ARPG_API AARPGAIController : public AAIController
{
	GENERATED_BODY()

public:
	virtual ETeamAttitude::Type GetTeamAttitudeTowards(const AActor& Other) const override
	{
		switch (GetTeamFromActor(&Other))
		{
		case ETeam::Player:
			return ETeamAttitude::Hostile;

		case ETeam::Enemy:
			return ETeamAttitude::Friendly;

		default:
			return ETeamAttitude::Neutral;
		}
	}
};
