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
  FORCEINLINE bool ShouldRotateRootBone() const { return bRotateRootBone; }
  FORCEINLINE bool IsElimmed() const { return bElimmed; }
  FORCEINLINE float GetHealth() const { return Health; }
  FORCEINLINE float GetMaxHealth() const { return MaxHealth; }

  void PlayFireMontage(bool bAiming);
  void Elim();
  void PlayElimMontage();
  UFUNCTION(NetMulticast, Reliable)
  void MulticastElim();

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
  void CalculateAO_Pitch();
  void SimProxiesTurn();
  float CalculateSpeed();

  virtual void OnRep_ReplicatedMovement() override;

  UFUNCTION()
  void OnRep_OverlappingWeapon(AWeapon* lastWeapon);

  UFUNCTION(Server, Reliable)
  void ServerEquipButtonOPressed();

  void HideCameraIfCharacterClose();
  void PlayHitReactMontage();
  UFUNCTION()
  void ReceiveDamage(AActor* DamagedActor, float Damage, const UDamageType* DamageType, class AController* InstigatorController, AActor* DamageCauser);
  void UpdateHUDHealth();
  // Poll for any relelvant classes and initialize our HUD
  void PollInit();
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
  UInputAction* StartFireAction = nullptr;
  UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Input, meta = (AllowPrivateAccess = "true"))
  UInputAction* EndFireAction = nullptr;
  UPROPERTY(EditAnywhere, Category = Combat)
  UAnimMontage* ElimMontage;

  bool bElimmed = false;
  
  FTimerHandle ElimTimer;

  UPROPERTY(EditDefaultsOnly)
  float ElimDelay = 3.f;

  void ElimTimerFinished();
  
  class ABlasterPlayerState* BlasterPlayerState;

  /**
  * Player health
  */

  UPROPERTY(EditAnywhere, Category = "Player Stats")
  float MaxHealth = 100.f;

  UPROPERTY(ReplicatedUsing = OnRep_Health, VisibleAnywhere, Category = "Player Stats")
  float Health = 100.f;

  UFUNCTION()
  void OnRep_Health();

  UPROPERTY(EditAnywhere)
  float CameraThreshold = 200.f;

  bool bRotateRootBone;
  float TurnThreshold = 0.5f;
  FRotator ProxyRotationLastFrame;
  FRotator ProxyRotation;
  float ProxyYaw;
  float TimeSinceLastMovementReplication;

  class ABlasterPlayerController* BlasterPlayerController;

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

  UPROPERTY(EditAnywhere, Category = Combat)
  UAnimMontage* HitReactMontage;
};
