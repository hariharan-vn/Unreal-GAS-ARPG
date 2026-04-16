#pragma once

UENUM()
enum class ETeam:uint8
{
	None = 0,
	Player = 1,
	Enemy = 2
};

static ETeam GetTeamFromActor(const AActor* Actor)
{
	const IGenericTeamAgentInterface* TeamAgent = Cast<IGenericTeamAgentInterface>(Actor);
	if (!TeamAgent) return ETeam::None;

	return static_cast<ETeam>(TeamAgent->GetGenericTeamId().GetId());
}