// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Announcment.generated.h"

/**
 * 
 */
UCLASS()
class BLASTER_API UAnnouncment : public UUserWidget
{
	GENERATED_BODY()
	
public:

	UPROPERTY(meta = (BindWidget))
	class UTextBlock* WarmupTime;
	UPROPERTY(meta = (BindWidget))
	class UTextBlock* AnnouncmentText;
	UPROPERTY(meta = (BindWidget))
	class UTextBlock* InfoText;
};
