// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "Blaster/BlasterTypes/TurningInPlace.h"
#include "Blaster/Interfaces/InteractWithCrosshairsInterface.h"
#include "BlasterCharacter.generated.h"

class USpringArmComponent;
class UCameraComponent;
class UInputMappingContext;
class UInputAction;
class UWidgetComponent;
struct FInputActionValue;
class AWeapon;
class UCombatComponent;

UCLASS()
class BLASTER_API ABlasterCharacter : public ACharacter, public IInteractWithCrosshairsInterface
{
  GENERATED_BODY()

public:
  ABlasterCharacter();
  virtual void Tick(float DeltaTime) override;
  virtual void SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent) override;
  virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
  virtual void PostInitializeComponents() override;

  void SetOverlappingWeapon(AWeapon* weapon);
  bool IsWeaponEquipped();
  bool IsAiming() const;
  FORCEINLINE float GetAO_Yaw() const { return AO_Yaw; }
  FORCEINLINE float GetAO_Pitch() const { return AO_Pitch; }
  AWeapon* GetEquippedWeapon();
  FORCEINLINE ETurningInPlace GetTurningInPlace() const { return TurningInPlace; }

  void PlayFireMontage(bool bAiming);

protected:
  virtual void BeginPlay() override;

  void Jump() override;
  void Move_Input(const FInputActionValue& Value);
  void Look_Input(const FInputActionValue& Value);
  void Equip_Input(const FInputActionValue& Value);
  void Crouch_Input(const FInputActionValue& Value);
  void UnCrouch_Input(const FInputActionValue& Value);
  void Aim_Input();
  void UnAim_Input();
  void StartFire_Input();
  void EndFire_Input();

  void AimOffset(float deltaTime);
  void TurnInPlace(float DeltaTime);

  UFUNCTION()
  void OnRep_OverlappingWeapon(AWeapon* lastWeapon);

  UFUNCTION(Server, Reliable)
  void ServerEquipButtonOPressed();

protected:

  // Input actions
  UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Input, meta = (AllowPrivateAccess = "true"))
  UInputMappingContext* DefaultMappingContext = nullptr;

  UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Input, meta = (AllowPrivateAccess = "true"))
  UInputAction* JumpAction = nullptr;
  UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Input, meta = (AllowPrivateAccess = "true"))
  UInputAction* MoveAction = nullptr;
  UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Input, meta = (AllowPrivateAccess = "true"))
  UInputAction* LookAction = nullptr;
  UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Input, meta = (AllowPrivateAccess = "true"))
  UInputAction* EquipAction = nullptr;
  UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Input, meta = (AllowPrivateAccess = "true"))
  UInputAction* CrouchAction = nullptr;
  UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Input, meta = (AllowPrivateAccess = "true"))
  UInputAction* UnCrouchAction = nullptr;
  UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Input, meta = (AllowPrivateAccess = "true"))
  UInputAction* AimAction = nullptr;
  UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Input, meta = (AllowPrivateAccess = "true"))
  UInputAction* UnAimAction = nullptr;
  UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Input, meta = (AllowPrivateAccess = "true"))
  UInputAction* FireAction = nullptr;

  //

  UPROPERTY(VisibleAnywhere, Category = Camera)
  USpringArmComponent* CameraBoom = nullptr;

  UPROPERTY(VisibleAnywhere, Category = Camera)
  UCameraComponent* FollowCamera = nullptr;

  UPROPERTY(EditAnywhere, BlueprintReadOnly, meta = (AllowPrivateAccess = "true"))
  UWidgetComponent* OverheadWidget = nullptr;

  UPROPERTY(VisibleAnywhere)
  UCombatComponent* Combat = nullptr;

  UPROPERTY(ReplicatedUsing = OnRep_OverlappingWeapon)
  AWeapon* OverlappingWeapon = nullptr;

  float InterpAO_Yaw;
  float AO_Yaw;
  float AO_Pitch;
  FRotator StartingAimRotation;

  ETurningInPlace TurningInPlace;

  UPROPERTY(EditAnywhere, Category = Combat)
  class UAnimMontage* FireWeaponMontage;
};
