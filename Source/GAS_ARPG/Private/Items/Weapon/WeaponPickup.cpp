// Fill out your copyright notice in the Description page of Project Settings.


#include "Items/Weapon/WeaponPickup.h"
#include "Components/SphereComponent.h"
#include "Character/GAS_ARPGCharacter.h"

AWeaponPickup::AWeaponPickup()
{
	PrimaryActorTick.bCanEverTick = false;

	WeaponMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("WeaponMesh"));
	SetRootComponent(WeaponMesh);
	WeaponMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);

	OverlapSphere = CreateDefaultSubobject<USphereComponent>(TEXT("OverlapSphere"));
	OverlapSphere->SetupAttachment(RootComponent);
	OverlapSphere->SetSphereRadius(100.f);

	// Only overlap with Pawns — ignore everything else
	OverlapSphere->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	OverlapSphere->SetCollisionObjectType(ECC_WorldDynamic);
	OverlapSphere->SetCollisionResponseToAllChannels(ECR_Ignore);
	OverlapSphere->SetCollisionResponseToChannel(ECC_Pawn, ECR_Overlap);
}

void AWeaponPickup::OnPickedUp()
{
	Destroy();
}

void AWeaponPickup::BeginPlay()
{
	Super::BeginPlay();

	OverlapSphere->OnComponentBeginOverlap.AddDynamic(this, &AWeaponPickup::OnSphereBeginOverlap);
	OverlapSphere->OnComponentEndOverlap.AddDynamic(this, &AWeaponPickup::OnSphereEndOverlap);
}

void AWeaponPickup::OnSphereBeginOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor,
                                         UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep,
                                         const FHitResult& SweepResult)
{

	UE_LOG(LogTemp, Log, TEXT("OnSphereBeginOverlap"));
	
	AGAS_ARPGCharacter* Player = Cast<AGAS_ARPGCharacter>(OtherActor);
	if (!Player) return;

	Player->SetNearbyWeapon(this);
}

void AWeaponPickup::OnSphereEndOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor,
                                       UPrimitiveComponent* OtherComp, int32 OtherBodyIndex)
{
	AGAS_ARPGCharacter* Player = Cast<AGAS_ARPGCharacter>(OtherActor);
	if (!Player) return;

	// Clear reference only if this is the weapon the player is near
	if (Player->GetNearbyWeapon() == this)
	{
		Player->SetNearbyWeapon(nullptr);
	}
}
