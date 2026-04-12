// Fill out your copyright notice in the Description page of Project Settings.


#include "Animation/Notifies/AN_SendGameplayEvent.h"

#include "AbilitySystemBlueprintLibrary.h"
#include "Abilities/GameplayAbilityTypes.h"

void UAN_SendGameplayEvent::Notify(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation,
                                   const FAnimNotifyEventReference& EventReference)
{
	Super::Notify(MeshComp, Animation, EventReference);

	if (!MeshComp) return;

	AActor* Owner = MeshComp->GetOwner();
	if (!Owner) return;

	if (!EventTag.IsValid())
	{
		UE_LOG(LogTemp, Warning, TEXT("[AN_SendGameplayEvent] EventTag not set on %s"), *GetNameSafe(Animation));
		return;
	}

	UAbilitySystemBlueprintLibrary::SendGameplayEventToActor(Owner, EventTag, FGameplayEventData());
}
