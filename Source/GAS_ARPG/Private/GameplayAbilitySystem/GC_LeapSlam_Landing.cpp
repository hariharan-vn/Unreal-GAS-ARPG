// Fill out your copyright notice in the Description page of Project Settings.


#include "GameplayAbilitySystem/GC_LeapSlam_Landing.h"

#include "NiagaraFunctionLibrary.h"
#include "Kismet/GameplayStatics.h"

bool UGC_LeapSlam_Landing::OnExecute_Implementation(AActor* MyTarget, const FGameplayCueParameters& Parameters) const
{
	const FVector Location = Parameters.Location;

	if (ImpactParticle)
	{
		UNiagaraFunctionLibrary::SpawnSystemAtLocation(MyTarget, ImpactParticle, Location);
	}

	if (ImpactSound)
	{
		UGameplayStatics::PlaySoundAtLocation(MyTarget, ImpactSound, Location);
	}

	if (ImpactCameraShake)
	{
		UGameplayStatics::PlayWorldCameraShake(MyTarget, ImpactCameraShake, Location, 0.f, CameraShakeRadius);
	}
	return true; 
}
