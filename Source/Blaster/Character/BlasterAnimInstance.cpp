// Fill out your copyright notice in the Description page of Project Settings.


#include "BlasterAnimInstance.h"
#include "BlasterCharacter.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "../DebugHelper.h"
#include "Kismet/KismetMathLibrary.h"
#include "Blaster/Weapon/Weapon.h"

void UBlasterAnimInstance::NativeInitializeAnimation()
{
  Super::NativeInitializeAnimation();

  BlasterCharacter = Cast<ABlasterCharacter>(TryGetPawnOwner());
}

void UBlasterAnimInstance::NativeUpdateAnimation(float DeltaTime)
{
  Super::NativeUpdateAnimation(DeltaTime);

  if (BlasterCharacter == nullptr)
  {
    BlasterCharacter = Cast<ABlasterCharacter>(TryGetPawnOwner());
  }
  if (BlasterCharacter == nullptr) return;

  FVector velocity = BlasterCharacter->GetVelocity();
  velocity.Z = 0.0;
  Speed = velocity.Size();

  bIsInAir = BlasterCharacter->GetCharacterMovement()->IsFalling();
  bIsAccelerating = BlasterCharacter->GetCharacterMovement()->GetCurrentAcceleration().Size() > 0.0;
  bWeaponEquipped = BlasterCharacter->IsWeaponEquipped();
  EquippedWeapon = BlasterCharacter->GetEquippedWeapon();
  bIsCrouched = BlasterCharacter->bIsCrouched;

  // Offset yaw for strafing
  FRotator aimRotation = BlasterCharacter->GetBaseAimRotation();
  FRotator movementRotation = UKismetMathLibrary::MakeRotFromX(BlasterCharacter->GetVelocity());
  FRotator deltaRot = UKismetMathLibrary::NormalizedDeltaRotator(movementRotation, aimRotation);
  DeltaRotation = FMath::RInterpTo(DeltaRotation, deltaRot, DeltaTime, 6.0f);
  YawOffset = DeltaRotation.Yaw;

  // Lean
  CharacterRotationLastFrame = CharacterRotation;
  CharacterRotation = BlasterCharacter->GetActorRotation();
  const FRotator delta = UKismetMathLibrary::NormalizedDeltaRotator(CharacterRotation, CharacterRotationLastFrame);
  const float target = delta.Yaw / DeltaTime;
  const float interp = FMath::FInterpTo(Lean, target, DeltaTime, 6.0f);
  Lean = FMath::Clamp(interp, -90.0f, 90.0f);

  bAiming = BlasterCharacter->IsAiming();
  TurningInPlace = BlasterCharacter->GetTurningInPlace();
  bRotateRootBone = BlasterCharacter->ShouldRotateRootBone();

  AO_Yaw = BlasterCharacter->GetAO_Yaw();
  AO_Pitch = BlasterCharacter->GetAO_Pitch();

  if (bWeaponEquipped && EquippedWeapon && EquippedWeapon->GetWeaponMesh() && BlasterCharacter->GetMesh())
  {
    LeftHandTransform = EquippedWeapon->GetWeaponMesh()->GetSocketTransform(FName("LeftHandSocket"), ERelativeTransformSpace::RTS_World);
    FVector OutPosition;
    FRotator OutRotation;
    BlasterCharacter->GetMesh()->TransformToBoneSpace(FName("hand_r"), LeftHandTransform.GetLocation(), FRotator::ZeroRotator, OutPosition, OutRotation);
    LeftHandTransform.SetLocation(OutPosition);
    LeftHandTransform.SetRotation(FQuat(OutRotation));
  }
  bElimmed = BlasterCharacter->IsElimmed();
}
