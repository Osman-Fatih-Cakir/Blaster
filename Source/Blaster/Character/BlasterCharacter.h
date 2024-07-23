// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
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
class BLASTER_API ABlasterCharacter : public ACharacter
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

protected:
	virtual void BeginPlay() override;

	void Move_Input(const FInputActionValue& Value);
	void Look_Input(const FInputActionValue& Value);
	void Equip_Input(const FInputActionValue& Value);

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

	//

	UPROPERTY(VisibleAnywhere, Category=Camera)
	USpringArmComponent* CameraBoom = nullptr;

	UPROPERTY(VisibleAnywhere, Category = Camera)
	UCameraComponent* FollowCamera = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, meta = (AllowPrivateAccess = "true"))
	UWidgetComponent* OverheadWidget = nullptr;

	UPROPERTY(VisibleAnywhere)
	UCombatComponent* Combat = nullptr;

	UPROPERTY(ReplicatedUsing = OnRep_OverlappingWeapon)
	AWeapon* OverlappingWeapon = nullptr;
};
