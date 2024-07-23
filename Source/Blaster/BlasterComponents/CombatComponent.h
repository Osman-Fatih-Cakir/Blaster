// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "CombatComponent.generated.h"

class ABlasterCharacter;
class AWeapon;

UCLASS( ClassGroup=(Custom), meta=(BlueprintSpawnableComponent) )
class BLASTER_API UCombatComponent : public UActorComponent
{
	friend ABlasterCharacter;

	GENERATED_BODY()

public:	
	UCombatComponent();

	void EquipWeapon(AWeapon* weapon);

protected:
	virtual void BeginPlay() override;

protected:
	ABlasterCharacter* Character = nullptr;
	AWeapon* EquippedWeapon = nullptr;
};