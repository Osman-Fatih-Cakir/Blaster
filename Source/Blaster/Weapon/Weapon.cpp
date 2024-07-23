// Fill out your copyright notice in the Description page of Project Settings.


#include "Weapon.h"
#include "Components/WidgetComponent.h"
#include "Blaster/Character/BlasterCharacter.h"
#include "Components/SphereComponent.h"
#include "Net/UnrealNetwork.h"

AWeapon::AWeapon()
{
  PrimaryActorTick.bCanEverTick = false;
  bReplicates = true;

  WeaponMesh = CreateDefaultSubobject<USkeletalMeshComponent>(TEXT("WeaponMesh"));
  WeaponMesh->SetupAttachment(RootComponent);
  SetRootComponent(WeaponMesh);

  WeaponMesh->SetCollisionResponseToAllChannels(ECollisionResponse::ECR_Block);
  WeaponMesh->SetCollisionResponseToChannel(ECollisionChannel::ECC_Pawn, ECollisionResponse::ECR_Ignore);
  WeaponMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);

  AreaSphere = CreateDefaultSubobject<USphereComponent>(TEXT("AreaSphere"));
  AreaSphere->SetupAttachment(RootComponent);
  AreaSphere->SetCollisionResponseToAllChannels(ECollisionResponse::ECR_Ignore);
  AreaSphere->SetCollisionEnabled(ECollisionEnabled::NoCollision);
  if (HasAuthority())
  {
    AreaSphere->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
    AreaSphere->SetCollisionResponseToChannel(ECollisionChannel::ECC_Pawn, ECollisionResponse::ECR_Overlap);
  }

  PickupWidget = CreateDefaultSubobject<UWidgetComponent>(TEXT("PickupWidget"));
  PickupWidget->SetupAttachment(RootComponent);
}

void AWeapon::BeginPlay()
{
  Super::BeginPlay();

  if (HasAuthority())
  {
    AreaSphere->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
    AreaSphere->SetCollisionResponseToChannel(ECollisionChannel::ECC_Pawn, ECollisionResponse::ECR_Overlap);
    AreaSphere->OnComponentBeginOverlap.AddDynamic(this, &AWeapon::OnSphereOverlap);
    AreaSphere->OnComponentEndOverlap.AddDynamic(this, &AWeapon::OnSphereEndOverlap);
  }
  if (PickupWidget)
  {
    PickupWidget->SetVisibility(false);
  }
}

void AWeapon::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
  Super::GetLifetimeReplicatedProps(OutLifetimeProps);

  DOREPLIFETIME(AWeapon, WeaponState);
}

void AWeapon::ShowPickupWidget(bool bShowWidget)
{
  if (PickupWidget)
  {
    PickupWidget->SetVisibility(bShowWidget);
  }
}

void AWeapon::SetWeaponState(EWeaponState state)
{
  WeaponState = state;

  switch (WeaponState)
  {
  case EWeaponState::EWS_Equipped:
    ShowPickupWidget(false);
    AreaSphere->SetCollisionEnabled(ECollisionEnabled::NoCollision);
    break;
  }
}

void AWeapon::OnSphereOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult)
{
  if (ABlasterCharacter* BlasterCharacter = Cast<ABlasterCharacter>(OtherActor))
  {
    BlasterCharacter->SetOverlappingWeapon(this);
  }
}

void AWeapon::OnSphereEndOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex)
{
  if (ABlasterCharacter* BlasterCharacter = Cast<ABlasterCharacter>(OtherActor))
  {
    BlasterCharacter->SetOverlappingWeapon(nullptr);
  }
}

void AWeapon::OnRep_WeaponState()
{
  switch (WeaponState)
  {
  case EWeaponState::EWS_Equipped:
    ShowPickupWidget(false);
    break;
  }
}
