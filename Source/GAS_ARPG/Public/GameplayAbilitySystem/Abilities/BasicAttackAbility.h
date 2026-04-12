#pragma once

#include "CoreMinimal.h"
#include "ARPGAbilityBase.h"
#include "BasicAttackAbility.generated.h"

class UAbilityTask_WaitGameplayEvent;
class UAbilityTask_PlayMontageAndWait;
class AActor;
class UAnimMontage;
class UGameplayEffect;
class UAbilityTask_TickTrace;

UCLASS()
class GAS_ARPG_API UBasicAttackAbility : public UARPGAbilityBase
{
	GENERATED_BODY()

public:
	UBasicAttackAbility();

protected:
	virtual void ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo,
	                             const FGameplayAbilityActivationInfo ActivationInfo,
	                             const FGameplayEventData* TriggerEventData) override;

	virtual void EndAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo,
	                        const FGameplayAbilityActivationInfo ActivationInfo, bool bReplicateEndAbility,
	                        bool bWasCancelled) override;

private:
	UFUNCTION()
	void OnMontageCompleted();

	UFUNCTION()
	void OnMontageCancelled();

	// ── Hit window callbacks ──────────────────
	// Fired by AN_SendGameplayEvent on montage
	UFUNCTION()
	void OnHitWindowOpen(FGameplayEventData Payload);

	UFUNCTION()
	void OnHitWindowClose(FGameplayEventData Payload);

	void ApplyDamageToTarget(AActor* Target) const;

	UFUNCTION()
	void OnTraceHit(const FHitResult& Hit);

	UPROPERTY()
	TObjectPtr<UAbilityTask_TickTrace> TraceTask;

	UPROPERTY(EditDefaultsOnly, Category = "LeapSlam|Effects")
	FGameplayTag DamageAmountTag;
	
	UPROPERTY(EditDefaultsOnly, Category = "Attack|Animation")
	TObjectPtr<UAnimMontage> AttackMontage;

	UPROPERTY(EditDefaultsOnly, Category = "Attack|Damage")
	TSubclassOf<UGameplayEffect> GE_Damage;

	UPROPERTY()
	TObjectPtr<UAbilityTask_PlayMontageAndWait> MontageTask;

	UPROPERTY()
	TObjectPtr<UAbilityTask_WaitGameplayEvent> OpenWindowTask;

	UPROPERTY()
	TObjectPtr<UAbilityTask_WaitGameplayEvent> CloseWindowTask;

	UPROPERTY(EditDefaultsOnly, Category = "Attack|Damage")
	float DamageAmount = 20.f;

	// Socket on weapon mesh — sweep starts here
	UPROPERTY(EditDefaultsOnly, Category = "Attack|Trace")
	FName WeaponTipSocket = "WeaponTip";

	UPROPERTY(EditDefaultsOnly, Category = "Attack|Trace")
	FName WeaponBaseSocket = "WeaponBase";

	// Radius of the sweep trace
	UPROPERTY(EditDefaultsOnly, Category = "Attack|Trace")
	float SweepRadius = 30.f;
};
