// Fill out your copyright notice in the Description page of Project Settings.


#include "BlasterAnimInstance.h"
#include "BlasterCharacter.h"
#include "GameFramework/CharacterMovementComponent.h"

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

  bIsCrouched = BlasterCharacter->bIsCrouched;
}
