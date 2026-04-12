#include "GameplayAbilitySystem/Tasks/AbilityTask_TickTrace.h"
#include "Items/Weapon/WeaponActor.h"
#include "GameFramework/Character.h"
#include "AbilitySystemComponent.h"
#include "AbilitySystemInterface.h"
#include "GAS_ARPG/GAS_ARPG.h"

UAbilityTask_TickTrace* UAbilityTask_TickTrace::CreateTickTrace(UGameplayAbility* OwningAbility,
                                                                FName InWeaponBaseSocket, FName InWeaponTipSocket,
                                                                float InSweepRadius)
{
	UAbilityTask_TickTrace* Task = NewAbilityTask<UAbilityTask_TickTrace>(OwningAbility);
	Task->WeaponBaseSocket = InWeaponBaseSocket;
	Task->WeaponTipSocket = InWeaponTipSocket;
	Task->SweepRadius = InSweepRadius;
	return Task;
}

void UAbilityTask_TickTrace::Activate()
{
	Super::Activate();

	HitActors.Empty();
	bTickingTask = true; // enables TickTask
}

void UAbilityTask_TickTrace::TickTask(float DeltaTime)
{
	Super::TickTask(DeltaTime);

	if (!EnsureWeapon())
	{
		UE_LOG(ARPG_Ability, Warning, TEXT("[%hs] No weapon — stopping trace"), __FUNCTION__);
		EndTask();
		return;
	}

	if (!WeaponActor.IsValid())
	{
		return;
	}

	FVector TraceStart;
	FVector TraceEnd;

	if (WeaponActor->HasSocket(WeaponBaseSocket) && WeaponActor->HasSocket(WeaponTipSocket))
	{
		TraceStart = WeaponActor->GetSocketLocation(WeaponBaseSocket);
		TraceEnd = WeaponActor->GetSocketLocation(WeaponTipSocket);
	}
	else
	{
		// Fallback
		const ACharacter* Avatar = Cast<ACharacter>(GetAvatarActor());
		TraceStart = Avatar->GetMesh()->GetSocketLocation("hand_r");
		TraceEnd = TraceStart + Avatar->GetActorForwardVector() * 80.f;
	}

	FCollisionQueryParams Params;
	Params.AddIgnoredActor(GetAvatarActor());

	// Ignore already hit actors this swing
	for (const TWeakObjectPtr<AActor>& Hit : HitActors)
	{
		if (Hit.IsValid())
			Params.AddIgnoredActor(Hit.Get());
	}

	TArray<FHitResult> Hits;
	GetWorld()->SweepMultiByChannel(Hits, TraceStart, TraceEnd, FQuat::Identity, ECC_Pawn,
	                                FCollisionShape::MakeSphere(SweepRadius), Params);

	for (const FHitResult& Hit : Hits)
	{
		if (!Cast<IAbilitySystemInterface>(Hit.GetActor()))
			continue;

		HitActors.AddUnique(Hit.GetActor());

		OnHit.Broadcast(Hit);
	}
}

void UAbilityTask_TickTrace::StopTrace()
{
	HitActors.Empty();
	EndTask();
}

bool UAbilityTask_TickTrace::EnsureWeapon()
{
	if (WeaponActor.IsValid())
	{
		return true;
	}

	const ACharacter* Avatar = Cast<ACharacter>(GetAvatarActor());
	if (!Avatar)
	{
		UE_LOG(ARPG_Ability, Warning, TEXT("[%hs] Character not valid"), __FUNCTION__);
		return false;
	}

	TArray<AActor*> Attached;
	Avatar->GetAttachedActors(Attached);

	for (AActor* Actor : Attached)
	{
		if (AWeaponActor* FoundWeapon = Cast<AWeaponActor>(Actor))
		{
			WeaponActor = FoundWeapon;
			return true;
		}
	}
	UE_LOG(ARPG_Ability, Warning, TEXT("[%hs] No WeaponActor found on avatar"), __FUNCTION__);
	return false;
}

void UAbilityTask_TickTrace::OnDestroy(bool bInOwnerFinished)
{
	HitActors.Empty();
	Super::OnDestroy(bInOwnerFinished);
}
