// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Blaster/HUD/BlasterHUD.h"
#include "Blaster/BlasterTypes/CombatState.h"
#include "CombatComponent.generated.h"

class ABlasterCharacter;
class AWeapon;

#define TRACE_LENGTH 80000.f

UCLASS( ClassGroup=(Custom), meta=(BlueprintSpawnableComponent) )
class BLASTER_API UCombatComponent : public UActorComponent
{
	friend ABlasterCharacter;

	GENERATED_BODY()

public:	
	UCombatComponent();
  void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	void EquipWeapon(AWeapon* weapon);

protected:
	virtual void BeginPlay() override;

	UFUNCTION()
	void OnRep_EquippedWeapon();

  void SetAiming(bool aiming);
	UFUNCTION(Server, Reliable)
	void ServerSetAiming(bool bIsAiming);
	void FireButtonPressed(bool bPressed);

	UFUNCTION(Server, Reliable)
	void ServerFire(const FVector_NetQuantize& TraceHitTarget);

	UFUNCTION(NetMulticast, Reliable)
	void MulticastFire(const FVector_NetQuantize& TraceHitTarget);

	void TraceUnderCrosshairs(FHitResult& TraceHitResult);
	void SetHUDCrosshairs(float DeltaTime);
	void Fire();
	bool CanFire();
	void Reload();
	UFUNCTION(BlueprintCallable)
	void HandleReload();
	UFUNCTION()
	void OnRep_CombatState();
	void StartFireTimer();
	void FireTimerFinished();
	UFUNCTION(BlueprintCallable)
	void FinishReloading();
	UFUNCTION(Server, Reliable)
	void ServerReload();

protected:
	UPROPERTY()
	ABlasterCharacter* Character = nullptr;
	UPROPERTY()
	class ABlasterPlayerController* Controller;
	UPROPERTY()
	class ABlasterHUD* HUD;

	UPROPERTY(ReplicatedUsing = OnRep_EquippedWeapon)
	AWeapon* EquippedWeapon = nullptr;

	UPROPERTY(Replicated)
	bool bAiming = false;

	UPROPERTY(EditAnywhere)
	float BaseWalkSpeed;

	UPROPERTY(EditAnywhere)
	float AimWalkSpeed;

	bool bFireButtonPressed;

	FVector HitTarget;

	FHUDPackage HUDPackage;

	/**
	* Automatic fire
	*/

	FTimerHandle FireTimer;
	bool bCanFire = true;

	UPROPERTY(ReplicatedUsing = OnRep_CombatState)
	ECombatState CombatState = ECombatState::ECS_Unoccupied;

};
