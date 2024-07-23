// Fill out your copyright notice in the Description page of Project Settings.


#include "OverheadWidget.h"
#include "Components/TextBlock.h"

void UOverheadWidget::SetDisplayText(FString textToDisplay)
{
  if (DisplayText)
  {
    DisplayText->SetText(FText::FromString(textToDisplay));
  }
}

void UOverheadWidget::ShowPlayerNetRole(APawn* inPawn)
{
  ENetRole remoteRole = inPawn->GetRemoteRole();
  FString role;
  switch (remoteRole)
  {
  case ENetRole::ROLE_Authority:
  {
    role = FString("Remote Role: Authority");
  }
  break;
  case ENetRole::ROLE_AutonomousProxy:
  {
    role = FString("Remote Role: Autonomous Proxy");
  }
  break;
  case ENetRole::ROLE_SimulatedProxy:
  {
    role = FString("Remote Role: Simulated Proxy");
  }
  break;
  case ENetRole::ROLE_None:
  {
    role = FString("Remote Role: None");
  }
  break;
  }

  SetDisplayText(role);
}

void UOverheadWidget::NativeDestruct()
{
  RemoveFromParent();
  Super::NativeDestruct();
}
