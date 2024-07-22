// Fill out your copyright notice in the Description page of Project Settings.


#include "LobbyGameMode.h"
#include "GameFramework/GameStateBase.h"

void ALobbyGameMode::PostLogin(APlayerController* newplayer)
{
  Super::PostLogin(newplayer);

  int32 numOfPlayers = GameState.Get()->PlayerArray.Num();
  if (numOfPlayers == 2)
  {
    if (UWorld* world = GetWorld())
    {
      bUseSeamlessTravel = true;
      world->ServerTravel(FString("/Game/Maps/BlasterMap?listen"));
    }
  }
}
