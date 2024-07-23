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

ABlasterCharacter::ABlasterCharacter()
{
  PrimaryActorTick.bCanEverTick = true;

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
}

void ABlasterCharacter::Tick(float DeltaTime)
{
  Super::Tick(DeltaTime);
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
  }
}

void ABlasterCharacter::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
  Super::GetLifetimeReplicatedProps(OutLifetimeProps);

  DOREPLIFETIME_CONDITION(ABlasterCharacter, OverlappingWeapon, COND_OwnerOnly);
}

void ABlasterCharacter::PostInitializeComponents()
{
  Super::PostInitializeComponents();

  if (Combat)
  {
    Combat->Character = this;
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
