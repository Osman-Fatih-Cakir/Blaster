// Fill out your copyright notice in the Description page of Project Settings.


#include "CombatComponent.h"
#include "../Weapon/Weapon.h"
#include "../Character/BlasterCharacter.h"
#include "Engine/SkeletalMeshSocket.h"
#include "Components/SphereComponent.h"

UCombatComponent::UCombatComponent()
{
  PrimaryComponentTick.bCanEverTick = false;
}

void UCombatComponent::BeginPlay()
{
  Super::BeginPlay();
}

void UCombatComponent::EquipWeapon(AWeapon* weapon)
{
  if (Character == nullptr || weapon == nullptr) return;

  EquippedWeapon = weapon;
  EquippedWeapon->SetWeaponState(EWeaponState::EWS_Equipped);
  const USkeletalMeshSocket* handSocket = Character->GetMesh()->GetSocketByName(FName("RightHandSocket"));
  if (handSocket)
  {
    handSocket->AttachActor(EquippedWeapon, Character->GetMesh());
  }
  EquippedWeapon->SetOwner(Character);
}
