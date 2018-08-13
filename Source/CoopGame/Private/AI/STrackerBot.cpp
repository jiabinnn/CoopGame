
// Fill out your copyright notice in the Description page of Project Settings.

#include "STrackerBot.h"
#include <AI/Navigation/NavigationSystem.h>
#include <AI/Navigation/NavigationPath.h>
#include <Kismet/GameplayStatics.h>
#include "GameFramework/Character.h"
#include "SHealthComponent.h"
#include <Kismet/GameplayStatics.h>
#include <DrawDebugHelpers.h>
#include "Particles/ParticleSystem.h"


// Sets default values
ASTrackerBot::ASTrackerBot()
{
 	// Set this pawn to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;

	MeshComp = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("MeshComp"));
	RootComponent = MeshComp;
	MeshComp->SetCanEverAffectNavigation(false);
	MeshComp->SetSimulatePhysics(true);

	HealthComp = CreateDefaultSubobject<USHealthComponent>(TEXT("HealthComp"));
	HealthComp->OnHealthChanged.AddDynamic(this, &ASTrackerBot::HandleTakeDamage);

	MovementForce = 10;
	RequiredDistanceToTarget = 100;
	bUseVelocityChange = true;

	ExplosionDamage = 100.f;
	ExplosionRadius = 200.f;
}

// Called when the game starts or when spawned
void ASTrackerBot::BeginPlay()
{
	Super::BeginPlay();
	
	NextPathPoint = GetNextPathPoint();

}

void ASTrackerBot::HandleTakeDamage(USHealthComponent* OwningHealthComp, float Health, float HealthDelta, 
	const class UDamageType* DamageType, class AController* InstigatedBy, AActor* DamageCauser)
{
	//Explode on hitpoint = 0
	if (Health <= 0&& ExplosionEffect)
	{
		SelfDestruct();

	}
	//TODO: Pulse the material on hit

	if(MatInst == nullptr)
	{
		MatInst = MeshComp->CreateAndSetMaterialInstanceDynamicFromMaterial(0, MeshComp->GetMaterial(0));
	}
	if (MatInst)
	{
		MatInst->SetScalarParameterValue("LastTimeDamageTaken", GetWorld()->TimeSeconds);
	}


	UE_LOG(LogTemp, Log, TEXT("Health is %s of %s"), *FString::SanitizeFloat(Health), *GetName());


}

FVector ASTrackerBot::GetNextPathPoint()
{
	ACharacter* PlayPawn = UGameplayStatics::GetPlayerCharacter(this, 0);

	UNavigationPath* NavPath = UNavigationSystem::FindPathToActorSynchronously(this, GetActorLocation(), PlayPawn);

	if (NavPath->PathPoints.Num() > 1)
	{
		return NavPath->PathPoints[1];
	}

	return GetActorLocation();

}

void ASTrackerBot::SelfDestruct()
{
	if (bExploded)
	{
		return;
	}
	
	bExploded = true;

	UGameplayStatics::SpawnEmitterAtLocation(GetWorld(), ExplosionEffect, GetActorLocation());

	TArray<AActor*>IgnoreActors;
	IgnoreActors.Add(this);

	UGameplayStatics::ApplyRadialDamage(this, ExplosionDamage, GetActorLocation(),
		ExplosionRadius, nullptr, IgnoreActors, this, GetInstigatorController(), true);

	DrawDebugSphere(GetWorld(), GetActorLocation(), ExplosionRadius, 12, FColor::Red, false, 2.f, 0, 1.f);

	Destroy();
}

// Called every frame
void ASTrackerBot::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	float DistanceToTarget = (GetActorLocation() - NextPathPoint).Size();

	if (DistanceToTarget <= RequiredDistanceToTarget)
	{
		NextPathPoint = GetNextPathPoint();

		DrawDebugSphere(GetWorld(), NextPathPoint, 30.f, 12, FColor::Yellow, false, 0.f);
	}
	else
	{
		FVector ForceDirection = NextPathPoint - GetActorLocation();
		ForceDirection.Normalize();
		ForceDirection *= MovementForce;

		MeshComp->AddImpulse(ForceDirection, NAME_None, bUseVelocityChange);

		DrawDebugDirectionalArrow(GetWorld(), GetActorLocation(), NextPathPoint, 32, FColor::Black, false, 0.f, 2, 2.f);
	}

	ACharacter* PlayPawn = UGameplayStatics::GetPlayerCharacter(this, 0);
	FVector PlayerLocation = PlayPawn->GetActorLocation();
	float Distance = (PlayerLocation - GetActorLocation()).Size();
	if (Distance <= 100)
	{
		SelfDestruct();
	}


}

