// Fill out your copyright notice in the Description page of Project Settings.


#include "CombatComponent.h"
#include "../Weapon/Weapon.h"
#include "../Character/BlasterCharacter.h"
#include "Engine/SkeletalMeshSocket.h"
#include "Components/SphereComponent.h"
#include "Net/UnrealNetwork.h"
#include "GameFramework/CharacterMovementComponent.h"

UCombatComponent::UCombatComponent()
{
  PrimaryComponentTick.bCanEverTick = false;

  BaseWalkSpeed = 600.f;
  AimWalkSpeed = 450.f;
}

void UCombatComponent::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
  Super::GetLifetimeReplicatedProps(OutLifetimeProps);

  DOREPLIFETIME(UCombatComponent, EquippedWeapon);
  DOREPLIFETIME(UCombatComponent, bAiming);
}

void UCombatComponent::BeginPlay()
{
  Super::BeginPlay();

  if (Character)
  {
    Character->GetCharacterMovement()->MaxWalkSpeed = BaseWalkSpeed;
  }
}

void UCombatComponent::OnRep_EquippedWeapon()
{
  if (EquippedWeapon && Character)
  {
    Character->GetCharacterMovement()->bOrientRotationToMovement = false;
    Character->bUseControllerRotationYaw = true;
  }
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
  Character->GetCharacterMovement()->bOrientRotationToMovement = false;
  Character->bUseControllerRotationYaw = true;
}

void UCombatComponent::SetAiming(bool aiming)
{
  bAiming = aiming;
  ServerSetAiming(bAiming);
  if (Character)
  {
    Character->GetCharacterMovement()->MaxWalkSpeed = bAiming ? AimWalkSpeed : BaseWalkSpeed;
  }
}

void UCombatComponent::ServerSetAiming_Implementation(bool bIsAiming)
{
  bAiming = bIsAiming;
  if (Character)
  {
    Character->GetCharacterMovement()->MaxWalkSpeed = bIsAiming ? AimWalkSpeed : BaseWalkSpeed;
  }
}

void UCombatComponent::FireButtonPressed(bool bPressed)
{
  bFireButtonPressed = bPressed;
  if (bFireButtonPressed)
  {
    ServerFire();
  }
}

void UCombatComponent::ServerFire_Implementation()
{
  MulticastFire();
}

void UCombatComponent::MulticastFire_Implementation()
{
  if (EquippedWeapon == nullptr) return;
  if (Character)
  {
    Character->PlayFireMontage(bAiming);
    EquippedWeapon->Fire();
  }
}
