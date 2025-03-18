// Fill out your copyright notice in the Description page of Project Settings.


#include "BlasterCharacter.h"
#include "GameFramework/SpringArmComponent.h"
#include "Camera/CameraComponent.h"
#include "EnhancedInputComponent.h"
#include "EnhancedInputSubsystems.h"
#include "InputActionValue.h"
#include "../HUD/OverheadWidget.h"
#include "Components/WidgetComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Net/UnrealNetwork.h"
#include "../Weapon/Weapon.h"
#include "../BlasterComponents/CombatComponent.h"
#include "../DebugHelper.h"
#include "Components/CapsuleComponent.h"
#include "Kismet/KismetMathLibrary.h"
#include "Blaster/PlayerController/BlasterPlayerController.h"
#include "BlasterAnimInstance.h"
#include "Blaster/Blaster.h"
#include "Blaster/GameMode/BlasterGameMode.h"
#include "TimerManager.h"

ABlasterCharacter::ABlasterCharacter()
{
  PrimaryActorTick.bCanEverTick = true;

  GetCapsuleComponent()->SetCollisionResponseToChannel(ECollisionChannel::ECC_Camera, ECollisionResponse::ECR_Ignore);
  GetMesh()->SetCollisionResponseToChannel(ECollisionChannel::ECC_Camera, ECollisionResponse::ECR_Ignore);

  CameraBoom = CreateDefaultSubobject<USpringArmComponent>(TEXT("CameraBoom"));
  CameraBoom->SetupAttachment(GetMesh());
  CameraBoom->TargetArmLength = 600.f;
  CameraBoom->bUsePawnControlRotation = true;

  FollowCamera = CreateDefaultSubobject<UCameraComponent>(TEXT("FollowCamera"));
  FollowCamera->SetupAttachment(CameraBoom, USpringArmComponent::SocketName);
  FollowCamera->bUsePawnControlRotation = false;

  bUseControllerRotationYaw = false;
  GetCharacterMovement()->bOrientRotationToMovement = true;

  OverheadWidget = CreateDefaultSubobject<UWidgetComponent>(TEXT("OverheadWidget"));
  OverheadWidget->SetupAttachment(RootComponent);

  Combat = CreateDefaultSubobject<UCombatComponent>(TEXT("CombatComponent"));
  Combat->SetIsReplicated(true);

  GetCharacterMovement()->NavAgentProps.bCanCrouch = true;
  GetCharacterMovement()->RotationRate = FRotator(0.f, 0.f, 850.f);

  TurningInPlace = ETurningInPlace::ETIP_NotTurning;
  NetUpdateFrequency = 66.0f;
  MinNetUpdateFrequency = 33.0f;

  GetMesh()->SetCollisionResponseToChannel(ECollisionChannel::ECC_Visibility, ECollisionResponse::ECR_Block);
  GetMesh()->SetCollisionObjectType(ECC_SkeletalMesh);
}

void ABlasterCharacter::BeginPlay()
{
  Super::BeginPlay();

  //Add Input Mapping Context
  if (APlayerController* PlayerController = Cast<APlayerController>(Controller))
  {
    if (UEnhancedInputLocalPlayerSubsystem* Subsystem = ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(PlayerController->GetLocalPlayer()))
    {
      Subsystem->AddMappingContext(DefaultMappingContext, 0);
    }
  }

  UpdateHUDHealth();
  if (HasAuthority())
  {
    OnTakeAnyDamage.AddDynamic(this, &ABlasterCharacter::ReceiveDamage);
  }
}

void ABlasterCharacter::Tick(float DeltaTime)
{
  Super::Tick(DeltaTime);

  if (GetLocalRole() > ENetRole::ROLE_SimulatedProxy && IsLocallyControlled())
  {
    AimOffset(DeltaTime);
  }
  else
  {
    TimeSinceLastMovementReplication += DeltaTime;
    if (TimeSinceLastMovementReplication > 0.25f)
    {
      OnRep_ReplicatedMovement();
    }
    CalculateAO_Pitch();
  }
  HideCameraIfCharacterClose();
}

void ABlasterCharacter::ReceiveDamage(AActor* DamagedActor, float Damage, const UDamageType* DamageType, AController* InstigatorController, AActor* DamageCauser)
{
  Health = FMath::Clamp(Health - Damage, 0.f, MaxHealth);
  UpdateHUDHealth();
  PlayHitReactMontage();

  if (Health == 0.f)
  {
    ABlasterGameMode* BlasterGameMode = GetWorld()->GetAuthGameMode<ABlasterGameMode>();
    if (BlasterGameMode)
    {
      BlasterPlayerController = BlasterPlayerController == nullptr ? Cast<ABlasterPlayerController>(Controller) : BlasterPlayerController;
      ABlasterPlayerController* AttackerController = Cast<ABlasterPlayerController>(InstigatorController);
      BlasterGameMode->PlayerEliminated(this, BlasterPlayerController, AttackerController);
    }
  }
}

void ABlasterCharacter::Elim()
{
  MulticastElim();
  GetWorldTimerManager().SetTimer(
    ElimTimer,
    this,
    &ABlasterCharacter::ElimTimerFinished,
    ElimDelay
  );
}

void ABlasterCharacter::MulticastElim_Implementation()
{
  bElimmed = true;
  PlayElimMontage();
}

void ABlasterCharacter::ElimTimerFinished()
{
  ABlasterGameMode* BlasterGameMode = GetWorld()->GetAuthGameMode<ABlasterGameMode>();
  if (BlasterGameMode)
  {
    BlasterGameMode->RequestRespawn(this, Controller);
  }
}

void ABlasterCharacter::PlayElimMontage()
{
  UAnimInstance* AnimInstance = GetMesh()->GetAnimInstance();
  if (AnimInstance && ElimMontage)
  {
    AnimInstance->Montage_Play(ElimMontage);
    FName SectionName("StartDie");
    AnimInstance->Montage_JumpToSection(SectionName);

    if (GetLocalRole() == ROLE_SimulatedProxy)
    {
      Debug::Print("Die");
    }
  }
}

float ABlasterCharacter::CalculateSpeed()
{
  FVector Velocity = GetVelocity();
  Velocity.Z = 0.f;
  return Velocity.Size();
}


void ABlasterCharacter::HideCameraIfCharacterClose()
{
  if (!IsLocallyControlled()) return;
  if ((FollowCamera->GetComponentLocation() - GetActorLocation()).Size() < CameraThreshold)
  {
    GetMesh()->SetVisibility(false);
    if (Combat && Combat->EquippedWeapon && Combat->EquippedWeapon->GetWeaponMesh())
    {
      Combat->EquippedWeapon->GetWeaponMesh()->bOwnerNoSee = true;
    }
  }
  else
  {
    GetMesh()->SetVisibility(true);
    if (Combat && Combat->EquippedWeapon && Combat->EquippedWeapon->GetWeaponMesh())
    {
      Combat->EquippedWeapon->GetWeaponMesh()->bOwnerNoSee = false;
    }
  }
}

void ABlasterCharacter::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
  Super::SetupPlayerInputComponent(PlayerInputComponent);

  if (UEnhancedInputComponent* EnhancedInputComponent = Cast<UEnhancedInputComponent>(PlayerInputComponent))
  {
    // Jumping
    EnhancedInputComponent->BindAction(JumpAction, ETriggerEvent::Started, this, &ACharacter::Jump);
    // Moving
    EnhancedInputComponent->BindAction(MoveAction, ETriggerEvent::Triggered, this, &ThisClass::Move_Input);
    // Looking
    EnhancedInputComponent->BindAction(LookAction, ETriggerEvent::Triggered, this, &ThisClass::Look_Input);
    // Equip
    EnhancedInputComponent->BindAction(EquipAction, ETriggerEvent::Started, this, &ThisClass::Equip_Input);
    // Crouch
    EnhancedInputComponent->BindAction(CrouchAction, ETriggerEvent::Triggered, this, &ThisClass::Crouch_Input);
    EnhancedInputComponent->BindAction(UnCrouchAction, ETriggerEvent::Triggered, this, &ThisClass::UnCrouch_Input);
    // aiming
    EnhancedInputComponent->BindAction(AimAction, ETriggerEvent::Triggered, this, &ThisClass::Aim_Input);
    EnhancedInputComponent->BindAction(UnAimAction, ETriggerEvent::Triggered, this, &ThisClass::UnAim_Input);

    PlayerInputComponent->BindAction("Jump", IE_Pressed, this, &ABlasterCharacter::Jump);
    EnhancedInputComponent->BindAction(StartFireAction, ETriggerEvent::Triggered, this, &ThisClass::StartFire_Input);
    EnhancedInputComponent->BindAction(EndFireAction, ETriggerEvent::Triggered, this, &ThisClass::EndFire_Input);
  }
}

void ABlasterCharacter::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
  Super::GetLifetimeReplicatedProps(OutLifetimeProps);

  DOREPLIFETIME_CONDITION(ABlasterCharacter, OverlappingWeapon, COND_OwnerOnly);
  DOREPLIFETIME(ABlasterCharacter, Health);
}

void ABlasterCharacter::PostInitializeComponents()
{
  Super::PostInitializeComponents();

  if (Combat)
  {
    Combat->Character = this;
  }
}

void ABlasterCharacter::PlayHitReactMontage()
{
  if (Combat == nullptr || Combat->EquippedWeapon == nullptr) return;

  UAnimInstance* AnimInstance = GetMesh()->GetAnimInstance();
  if (AnimInstance && HitReactMontage)
  {
    AnimInstance->Montage_Play(HitReactMontage);
    FName SectionName("FromFront");
    AnimInstance->Montage_JumpToSection(SectionName);
  }
}

void ABlasterCharacter::Jump()
{
  if (bIsCrouched)
  {
    UnCrouch();
  }
  else
  {
    Super::Jump();
  }
}

void ABlasterCharacter::Move_Input(const FInputActionValue& Value)
{  // input is a Vector2D
  FVector2D movementVector = Value.Get<FVector2D>();

  if (Controller != nullptr)
  {
    // find out which way is forward
    const FRotator Rotation = Controller->GetControlRotation();
    const FRotator YawRotation(0, Rotation.Yaw, 0);

    // get forward vector
    const FVector ForwardDirection = FRotationMatrix(YawRotation).GetUnitAxis(EAxis::X);

    // get right vector
    const FVector RightDirection = FRotationMatrix(YawRotation).GetUnitAxis(EAxis::Y);

    // add movement
    AddMovementInput(ForwardDirection, movementVector.Y);
    AddMovementInput(RightDirection, movementVector.X);
  }
}

void ABlasterCharacter::Look_Input(const FInputActionValue& Value)
{
  // input is a Vector2D
  FVector2D lookAxisVector = Value.Get<FVector2D>();

  if (Controller != nullptr)
  {
    AddControllerYawInput(lookAxisVector.X);
    AddControllerPitchInput(lookAxisVector.Y);
  }
}

void ABlasterCharacter::Equip_Input(const FInputActionValue& Value)
{
  if (Combat)
  {
    if (HasAuthority())
    {
      // server
      Combat->EquipWeapon(OverlappingWeapon);
    }
    else
    {
      // client
      ServerEquipButtonOPressed();
    }
  }
}

void ABlasterCharacter::Crouch_Input(const FInputActionValue& Value)
{
  Crouch();
}

void ABlasterCharacter::UnCrouch_Input(const FInputActionValue& Value)
{
  UnCrouch();
}

void ABlasterCharacter::Aim_Input()
{
  if (Combat)
    Combat->SetAiming(true);
}

void ABlasterCharacter::UnAim_Input()
{
  if (Combat)
    Combat->SetAiming(false);
}

void ABlasterCharacter::StartFire_Input()
{
  if (Combat)
  {
    Combat->FireButtonPressed(true);
  }
}

void ABlasterCharacter::EndFire_Input()
{
  if (Combat)
  {
    Combat->FireButtonPressed(false);
  }
}

void ABlasterCharacter::AimOffset(float deltaTime)
{
  if (Combat == nullptr || Combat->EquippedWeapon == nullptr) return;

  float Speed = CalculateSpeed();
  bool bIsInAir = GetCharacterMovement()->IsFalling();

  if (FMath::Abs(Speed) < 0.01f && !bIsInAir) // standing still, not jumping
  {
    bRotateRootBone = true;
    FRotator CurrentAimRotation = FRotator(0.f, GetBaseAimRotation().Yaw, 0.f);
    FRotator DeltaAimRotation = UKismetMathLibrary::NormalizedDeltaRotator(CurrentAimRotation, StartingAimRotation);
    AO_Yaw = DeltaAimRotation.Yaw;
    if (TurningInPlace == ETurningInPlace::ETIP_NotTurning)
    {
      InterpAO_Yaw = AO_Yaw;
    }
    bUseControllerRotationYaw = true;
    TurnInPlace(deltaTime);
  }
  if (Speed > 0.f || bIsInAir) // running, or jumping
  {
    bRotateRootBone = false;
    StartingAimRotation = FRotator(0.f, GetBaseAimRotation().Yaw, 0.f);
    AO_Yaw = 0.f;
    bUseControllerRotationYaw = true;
    TurningInPlace = ETurningInPlace::ETIP_NotTurning;
  }

  CalculateAO_Pitch();
}

void ABlasterCharacter::CalculateAO_Pitch()
{
  AO_Pitch = GetBaseAimRotation().Pitch;
  if (AO_Pitch > 90.0f && !IsLocallyControlled())
  {
    // while ue compressing & sending rotation values over network, negative values become positive (ex: -90 -> 270). Fixing this here.
    // map pitch from [270, 360] to [-90, 0]
    AO_Pitch = FMath::GetMappedRangeValueClamped(FVector2D(270.f, 360.f), FVector2D(-90.f, 0.f), AO_Pitch);
  }
}

void ABlasterCharacter::SimProxiesTurn()
{
  if (Combat == nullptr || Combat->EquippedWeapon == nullptr) return;
  bRotateRootBone = false;
  float Speed = CalculateSpeed();
  if (Speed > 0.f)
  {
    TurningInPlace = ETurningInPlace::ETIP_NotTurning;
    return;
  }

  ProxyRotationLastFrame = ProxyRotation;
  ProxyRotation = GetActorRotation();
  ProxyYaw = UKismetMathLibrary::NormalizedDeltaRotator(ProxyRotation, ProxyRotationLastFrame).Yaw;

  UE_LOG(LogTemp, Warning, TEXT("ProxyYaw: %f"), ProxyYaw);

  if (FMath::Abs(ProxyYaw) > TurnThreshold)
  {
    if (ProxyYaw > TurnThreshold)
    {
      TurningInPlace = ETurningInPlace::ETIP_Right;
    }
    else if (ProxyYaw < -TurnThreshold)
    {
      TurningInPlace = ETurningInPlace::ETIP_Left;
    }
    else
    {
      TurningInPlace = ETurningInPlace::ETIP_NotTurning;
    }
    return;
  }
  TurningInPlace = ETurningInPlace::ETIP_NotTurning;

}

void ABlasterCharacter::TurnInPlace(float DeltaTime)
{
  if (AO_Yaw > 90.f)
  {
    TurningInPlace = ETurningInPlace::ETIP_Right;
  }
  else if (AO_Yaw < -90.f)
  {
    TurningInPlace = ETurningInPlace::ETIP_Left;
  }
  if (TurningInPlace != ETurningInPlace::ETIP_NotTurning)
  {
    InterpAO_Yaw = FMath::FInterpTo(InterpAO_Yaw, 0.f, DeltaTime, 4.f);
    AO_Yaw = InterpAO_Yaw;
    if (FMath::Abs(AO_Yaw) < 15.f)
    {
      TurningInPlace = ETurningInPlace::ETIP_NotTurning;
      StartingAimRotation = FRotator(0.f, GetBaseAimRotation().Yaw, 0.f);
    }
  }
}

void ABlasterCharacter::OnRep_ReplicatedMovement()
{
  Super::OnRep_ReplicatedMovement();
  SimProxiesTurn();
  TimeSinceLastMovementReplication = 0.f;
}

void ABlasterCharacter::OnRep_Health()
{
  UpdateHUDHealth();
  PlayHitReactMontage();
}

void ABlasterCharacter::UpdateHUDHealth()
{
  BlasterPlayerController = BlasterPlayerController == nullptr ? Cast<ABlasterPlayerController>(Controller) : BlasterPlayerController;
  if (BlasterPlayerController)
  {
    BlasterPlayerController->SetHUDHealth(Health, MaxHealth);
  }
}

void ABlasterCharacter::PlayFireMontage(bool bAiming)
{
  if (Combat == nullptr || Combat->EquippedWeapon == nullptr) return;

  UAnimInstance* AnimInstance = GetMesh()->GetAnimInstance();
  if (AnimInstance && FireWeaponMontage)
  {
    AnimInstance->Montage_Play(FireWeaponMontage);
    FName SectionName;
    SectionName = bAiming ? FName("RifleAim") : FName("RifleHip");
    AnimInstance->Montage_JumpToSection(SectionName);
  }
}

void ABlasterCharacter::SetOverlappingWeapon(AWeapon* weapon)
{
  if (weapon == nullptr && OverlappingWeapon)
  {
    OverlappingWeapon->ShowPickupWidget(false);
  }

  OverlappingWeapon = weapon;

  if (IsLocallyControlled())
  {
    // OnRep_OverlappingWeapon won't be called if we are on server. So we do the same logic here
    if (OverlappingWeapon)
    {
      OverlappingWeapon->ShowPickupWidget(true);
    }
  }
}

bool ABlasterCharacter::IsWeaponEquipped()
{
  return (Combat && Combat->EquippedWeapon != nullptr);
}

bool ABlasterCharacter::IsAiming() const
{
  return (Combat && Combat->bAiming);
}

AWeapon* ABlasterCharacter::GetEquippedWeapon()
{
  if (Combat == nullptr) return nullptr;
  return Combat->EquippedWeapon;
}

void ABlasterCharacter::OnRep_OverlappingWeapon(AWeapon* lastWeapon)
{
  if (OverlappingWeapon)
  {
    OverlappingWeapon->ShowPickupWidget(true);
  }

  if (lastWeapon && OverlappingWeapon == nullptr)
  {
    lastWeapon->ShowPickupWidget(false);
  }
}

void ABlasterCharacter::ServerEquipButtonOPressed_Implementation()
{
  if (Combat)
  {
    Combat->EquipWeapon(OverlappingWeapon);
  }
}
