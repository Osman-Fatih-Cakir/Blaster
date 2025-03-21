// Fill out your copyright notice in the Description page of Project Settings.


#include "Weapon.h"
#include "Components/WidgetComponent.h"
#include "Blaster/Character/BlasterCharacter.h"
#include "Components/SphereComponent.h"
#include "Net/UnrealNetwork.h"
#include "Animation/AnimationAsset.h"
#include "Components/SkeletalMeshComponent.h"
#include "Casing.h"
#include "Engine/SkeletalMeshSocket.h"
#include "Blaster/PlayerController/BlasterPlayerController.h"

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
  DOREPLIFETIME(AWeapon, Ammo);
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
    WeaponMesh->SetSimulatePhysics(false);
    WeaponMesh->SetEnableGravity(false);
    WeaponMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
    break;
  case EWeaponState::EWS_Dropped:
    if (HasAuthority())
    {
      AreaSphere->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
    }
    WeaponMesh->SetSimulatePhysics(true);
    WeaponMesh->SetEnableGravity(true);
    WeaponMesh->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
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
    WeaponMesh->SetSimulatePhysics(false);
    WeaponMesh->SetEnableGravity(false);
    WeaponMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
    break;
  case EWeaponState::EWS_Dropped:
    WeaponMesh->SetSimulatePhysics(true);
    WeaponMesh->SetEnableGravity(true);
    WeaponMesh->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
    break;
  }
}

void AWeapon::Fire(const FVector& HitTarget)
{
  if (FireAnimation)
  {
    WeaponMesh->PlayAnimation(FireAnimation, false);
  }
  if (CasingClass)
  {
    const USkeletalMeshSocket* AmmoEjectSocket = WeaponMesh->GetSocketByName(FName("AmmoEject"));
    if (AmmoEjectSocket)
    {
      FTransform SocketTransform = AmmoEjectSocket->GetSocketTransform(WeaponMesh);

      UWorld* World = GetWorld();
      if (World)
      {
        World->SpawnActor<ACasing>(
          CasingClass,
          SocketTransform.GetLocation(),
          SocketTransform.GetRotation().Rotator()
        );
      }
    }
  }
  SpendRound();
}

void AWeapon::Dropped()
{
  SetWeaponState(EWeaponState::EWS_Dropped);
  FDetachmentTransformRules DetachRules(EDetachmentRule::KeepWorld, true);
  WeaponMesh->DetachFromComponent(DetachRules);
  SetOwner(nullptr);
  BlasterOwnerCharacter = nullptr;
  BlasterOwnerController = nullptr;
}

void AWeapon::SetHUDAmmo()
{
  BlasterOwnerCharacter = BlasterOwnerCharacter == nullptr ? Cast<ABlasterCharacter>(GetOwner()) : BlasterOwnerCharacter;
  if (BlasterOwnerCharacter)
  {
    BlasterOwnerController = BlasterOwnerController == nullptr ? Cast<ABlasterPlayerController>(BlasterOwnerCharacter->Controller) : BlasterOwnerController;
    if (BlasterOwnerController)
    {
      BlasterOwnerController->SetHUDWeaponAmmo(Ammo);
    }
  }
}

void AWeapon::SpendRound()
{
  Ammo = FMath::Clamp(Ammo - 1, 0, MagCapacity);
  SetHUDAmmo();
}

void AWeapon::OnRep_Ammo()
{
  BlasterOwnerCharacter = BlasterOwnerCharacter == nullptr ? Cast<ABlasterCharacter>(GetOwner()) : BlasterOwnerCharacter;
  SetHUDAmmo();
}

void AWeapon::OnRep_Owner()
{
  Super::OnRep_Owner();
  if (Owner == nullptr)
  {
    BlasterOwnerCharacter = nullptr;
    BlasterOwnerController = nullptr;
  }
  else
  {
    SetHUDAmmo();
  }
}

bool AWeapon::IsEmpty()
{
  return Ammo <= 0;
}

void AWeapon::SetAmmo(float ammo)
{
  Ammo = ammo;
  SetHUDAmmo();
}
