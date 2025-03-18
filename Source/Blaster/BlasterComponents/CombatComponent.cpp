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
#include "Blaster/PlayerController/BlasterPlayerController.h"
#include "Blaster/HUD/BlasterHUD.h"
#include "../DebugHelper.h"
#include "TimerManager.h"

UCombatComponent::UCombatComponent()
{
  PrimaryComponentTick.bCanEverTick = true;

  BaseWalkSpeed = 600.f;
  AimWalkSpeed = 450.f;
}

void UCombatComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
  Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

  if (Character && Character->IsLocallyControlled())
  {
    FHitResult HitResult;
    TraceUnderCrosshairs(HitResult);
    HitTarget = HitResult.ImpactPoint;

    SetHUDCrosshairs(DeltaTime);
  }
}

void UCombatComponent::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
  Super::GetLifetimeReplicatedProps(OutLifetimeProps);

  DOREPLIFETIME(UCombatComponent, EquippedWeapon);
  DOREPLIFETIME(UCombatComponent, bAiming);
  DOREPLIFETIME(UCombatComponent, CombatState);
}

void UCombatComponent::BeginPlay()
{
  Super::BeginPlay();

  if (Character)
  {
    Character->GetCharacterMovement()->MaxWalkSpeed = BaseWalkSpeed;
  }
}

void UCombatComponent::SetHUDCrosshairs(float DeltaTime)
{
  if (Character == nullptr || Character->Controller == nullptr) return;

  Controller = Controller == nullptr ? Cast<ABlasterPlayerController>(Character->Controller) : Controller;
  if (Controller)
  {
    HUD = HUD == nullptr ? Cast<ABlasterHUD>(Controller->GetHUD()) : HUD;
    if (HUD)
    {
      if (EquippedWeapon)
      {
        HUDPackage.CrosshairsCenter = EquippedWeapon->CrosshairsCenter;
        HUDPackage.CrosshairsLeft = EquippedWeapon->CrosshairsLeft;
        HUDPackage.CrosshairsRight = EquippedWeapon->CrosshairsRight;
        HUDPackage.CrosshairsBottom = EquippedWeapon->CrosshairsBottom;
        HUDPackage.CrosshairsTop = EquippedWeapon->CrosshairsTop;
      }
      else
      {
        HUDPackage.CrosshairsCenter = nullptr;
        HUDPackage.CrosshairsLeft = nullptr;
        HUDPackage.CrosshairsRight = nullptr;
        HUDPackage.CrosshairsBottom = nullptr;
        HUDPackage.CrosshairsTop = nullptr;
      }
      HUD->SetHUDPackage(HUDPackage);
    }
  }
}

void UCombatComponent::OnRep_EquippedWeapon()
{
  if (EquippedWeapon && Character)
  {
    EquippedWeapon->SetWeaponState(EWeaponState::EWS_Equipped);
    const USkeletalMeshSocket* HandSocket = Character->GetMesh()->GetSocketByName(FName("RightHandSocket"));
    if (HandSocket)
    {
      HandSocket->AttachActor(EquippedWeapon, Character->GetMesh());
    }
    Character->GetCharacterMovement()->bOrientRotationToMovement = false;
    Character->bUseControllerRotationYaw = true;
  }
}

void UCombatComponent::EquipWeapon(AWeapon* weapon)
{
  if (Character == nullptr || weapon == nullptr) return;
  if (EquippedWeapon)
  {
    EquippedWeapon->Dropped();
  }
  EquippedWeapon = weapon;
  EquippedWeapon->SetWeaponState(EWeaponState::EWS_Equipped);
  const USkeletalMeshSocket* handSocket = Character->GetMesh()->GetSocketByName(FName("RightHandSocket"));
  if (handSocket)
  {
    handSocket->AttachActor(EquippedWeapon, Character->GetMesh());
  }
  EquippedWeapon->SetOwner(Character);
  EquippedWeapon->SetHUDAmmo();
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
  if (bFireButtonPressed && EquippedWeapon)
  {
    Fire();
  }
}

void UCombatComponent::Fire()
{
  if (CanFire())
  {
    bCanFire = false;
    ServerFire(HitTarget);
    StartFireTimer();
  }
}

bool UCombatComponent::CanFire()
{
  if (EquippedWeapon == nullptr) return false;
  return !EquippedWeapon->IsEmpty() || !bCanFire;
}

void UCombatComponent::StartFireTimer()
{
  if (EquippedWeapon == nullptr || Character == nullptr) return;
  Character->GetWorldTimerManager().SetTimer(
    FireTimer,
    this,
    &UCombatComponent::FireTimerFinished,
    EquippedWeapon->FireDelay
  );
}

void UCombatComponent::FireTimerFinished()
{
  if (EquippedWeapon == nullptr) return;
  bCanFire = true;
  if (bFireButtonPressed && EquippedWeapon->bAutomatic)
  {
    Fire();
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

void UCombatComponent::Reload()
{
  if (CombatState != ECombatState::ECS_Reloading)
    ServerReload();
}

void UCombatComponent::ServerReload_Implementation()
{
  if (Character == nullptr) return;

  CombatState = ECombatState::ECS_Reloading;
  HandleReload();
}

void UCombatComponent::FinishReloading()
{
  if (Character == nullptr) return;
  if (Character->HasAuthority())
  {
    CombatState = ECombatState::ECS_Unoccupied;
  }
}

void UCombatComponent::OnRep_CombatState()
{
  switch (CombatState)
  {
  case ECombatState::ECS_Reloading:
    HandleReload();
    break;
  }
}

void UCombatComponent::HandleReload()
{
  Character->PlayReloadMontage();
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
    if (Character)
    {
      float DistanceToCharacter = (Character->GetActorLocation() - Start).Size();
      Start += CrosshairWorldDirection * (DistanceToCharacter + 100.f);
    }
    FVector End = Start + CrosshairWorldDirection * TRACE_LENGTH;

    GetWorld()->LineTraceSingleByChannel(
      TraceHitResult,
      Start,
      End,
      ECollisionChannel::ECC_Visibility
    );

    if (TraceHitResult.GetActor() && TraceHitResult.GetActor()->Implements<UInteractWithCrosshairsInterface>())
    {
      HUDPackage.CrosshairsColor = FLinearColor::Red;
    }
    else
    {
      HUDPackage.CrosshairsColor = FLinearColor::White;
    }
  }
}