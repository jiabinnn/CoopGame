// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "SPickUpActor.generated.h"
class USphereComponent;
class UDecalComponent;
class ASPowerupActor;

UCLASS()
class COOPGAME_API ASPickUpActor : public AActor
{
	GENERATED_BODY()
	
public:	
	// Sets default values for this actor's properties
	ASPickUpActor();

protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

	UPROPERTY(VisibleAnywhere, Category = "Components")
	USphereComponent* SphereComp;

	UPROPERTY(VisibleAnywhere, Category = "Components")
	UDecalComponent* DecalComp;

	UPROPERTY(EditDefaultsOnly, Category = "PickupActor")
	TSubclassOf<ASPowerupActor> PowerupClass;

	ASPowerupActor* PowerupInstance;

	UFUNCTION()
	void Respawn();

	FTimerHandle TimerHandle_RespawnTimer;

	UPROPERTY(EditDefaultsOnly, Category = "PickupActor")
	float CooldownDuration;
public:

	virtual void NotifyActorBeginOverlap(AActor* OtherActor) override;
	
};
