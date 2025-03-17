// Fill out your copyright notice in the Description page of Project Settings.


#include "CombatComponent.h"
#include "../Weapon/Weapon.h"
#include "../Character/BlasterCharacter.h"
#include "Engine/SkeletalMeshSocket.h"
#include "Components/SphereComponent.h"
#include "Net/UnrealNetwork.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Kismet/GameplayStatics.h"
#include "DrawDebugHelpers.h"

UCombatComponent::UCombatComponent()
{
  PrimaryComponentTick.bCanEverTick = true;

  BaseWalkSpeed = 600.f;
  AimWalkSpeed = 450.f;
}

void UCombatComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
  Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

  FHitResult HitResult;
  TraceUnderCrosshairs(HitResult);
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
    FHitResult HitResult;
    TraceUnderCrosshairs(HitResult);
    ServerFire(HitResult.ImpactPoint);
  }
}

void UCombatComponent::ServerFire_Implementation(const FVector_NetQuantize& TraceHitTarget)
{
  MulticastFire(TraceHitTarget);
}

void UCombatComponent::MulticastFire_Implementation(const FVector_NetQuantize& TraceHitTarget)
{
  if (EquippedWeapon == nullptr) return;
  if (Character)
  {
    Character->PlayFireMontage(bAiming);
    EquippedWeapon->Fire(TraceHitTarget);
  }
}

void UCombatComponent::TraceUnderCrosshairs(FHitResult& TraceHitResult)
{
  FVector2D ViewportSize;
  if (GEngine && GEngine->GameViewport)
  {
    GEngine->GameViewport->GetViewportSize(ViewportSize);
  }

  FVector2D CrosshairLocation(ViewportSize.X / 2.f, ViewportSize.Y / 2.f);
  FVector CrosshairWorldPosition;
  FVector CrosshairWorldDirection;
  bool bScreenToWorld = UGameplayStatics::DeprojectScreenToWorld(
    UGameplayStatics::GetPlayerController(this, 0),
    CrosshairLocation,
    CrosshairWorldPosition,
    CrosshairWorldDirection
  );

  if (bScreenToWorld)
  {
    FVector Start = CrosshairWorldPosition;

    FVector End = Start + CrosshairWorldDirection * TRACE_LENGTH;

    GetWorld()->LineTraceSingleByChannel(
      TraceHitResult,
      Start,
      End,
      ECollisionChannel::ECC_Visibility
    );
  }
}