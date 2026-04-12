#pragma once

#include "CoreMinimal.h"
#include "Abilities/Tasks/AbilityTask.h"
#include "AbilityTask_TickTrace.generated.h"

class AWeaponActor;
class AActor;

UCLASS()
class GAS_ARPG_API UAbilityTask_TickTrace : public UAbilityTask
{
	GENERATED_BODY()
	DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnTraceHit, const FHitResult&, Hit);

public:
	UPROPERTY(BlueprintAssignable)
	FOnTraceHit OnHit;

	UFUNCTION(BlueprintCallable, Category = "Ability|Tasks",
		meta = (HidePin = "OwningAbility", DefaultToSelf = "OwningAbility"))
	static UAbilityTask_TickTrace* CreateTickTrace(UGameplayAbility* OwningAbility, FName InWeaponBaseSocket,
	                                               FName InWeaponTipSocket, float InSweepRadius);

	virtual void Activate() override;
	virtual void TickTask(float DeltaTime) override;
	virtual void OnDestroy(bool bInOwnerFinished) override;

	// Called by ability when window closes
	void StopTrace();

private:
	bool EnsureWeapon();

	FName WeaponBaseSocket;
	FName WeaponTipSocket;
	float SweepRadius;

	TArray<TWeakObjectPtr<AActor>> HitActors;

	TWeakObjectPtr<AWeaponActor> WeaponActor;
};
