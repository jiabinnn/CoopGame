
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
#include "Components/SphereComponent.h"
#include "SCharacter.h"
#include "Sound/SoundCue.h"

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

	SphereComp = CreateDefaultSubobject<USphereComponent>(TEXT("SphereComp"));
	SphereComp->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	SphereComp->SetCollisionResponseToAllChannels(ECR_Ignore);
	SphereComp->SetCollisionResponseToChannel(ECC_Pawn, ECR_Overlap);
	SphereComp->SetSphereRadius(200.f);
	SphereComp->SetupAttachment(RootComponent);

	MovementForce = 10;
	RequiredDistanceToTarget = 100;
	bUseVelocityChange = true;

	SelfDamageInterval = 0.25;
	ExplosionDamage = 50.f;
	ExplosionRadius = 200.f;
}

// Called when the game starts or when spawned
void ASTrackerBot::BeginPlay()
{
	Super::BeginPlay();
	
	if(Role == ROLE_Authority)
	{
		NextPathPoint = GetNextPathPoint();

		FTimerHandle TimerHandle_CheckPowerLevel;
		GetWorldTimerManager().SetTimer(TimerHandle_CheckPowerLevel, this, &ASTrackerBot::OnCheckNearbyBots, 1.f, true);
	}

}

void ASTrackerBot::HandleTakeDamage(USHealthComponent* OwningHealthComp, float Health, float HealthDelta, 
	const class UDamageType* DamageType, class AController* InstigatedBy, AActor* DamageCauser)
{
	//Explode on hitpoint = 0
	if (Health <= 0)
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

	if (NavPath && NavPath->PathPoints.Num() > 1)
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
	if(ExplosionEffect)
	{
		UGameplayStatics::SpawnEmitterAtLocation(GetWorld(), ExplosionEffect, GetActorLocation());
	}
	UGameplayStatics::PlaySoundAtLocation(this, ExplosionSound, GetActorLocation());
	
	if(Role == ROLE_Authority)
	{ 
		TArray<AActor*>IgnoreActors;
		IgnoreActors.Add(this);

		float ActualDamage = ExplosionDamage + (ExplosionDamage * PowerLevel);
		UGameplayStatics::ApplyRadialDamage(this, ActualDamage, GetActorLocation(),
			ExplosionRadius, nullptr, IgnoreActors, this, GetInstigatorController(), true);

		DrawDebugSphere(GetWorld(), GetActorLocation(), ExplosionRadius, 12, FColor::Red, false, 2.f, 0, 1.f);
		SetLifeSpan(0.1);
	}
}

void ASTrackerBot::DamageSelf()
{
	UGameplayStatics::ApplyDamage(this, 20.f, GetInstigatorController(), this, nullptr);
}

void ASTrackerBot::OnCheckNearbyBots()
{
	const float Radius = 600.f;

	FCollisionShape CollShape;
	CollShape.SetSphere(Radius);

	FCollisionObjectQueryParams QurayParams;
	QurayParams.AddObjectTypesToQuery(ECC_PhysicsBody);
	QurayParams.AddObjectTypesToQuery(ECC_Pawn);

	TArray<FOverlapResult> Overlaps;
	GetWorld()->OverlapMultiByObjectType(Overlaps, GetActorLocation(), FQuat::Identity, QurayParams, CollShape);

	DrawDebugSphere(GetWorld(), GetActorLocation(), Radius, 12, FColor::White, false, 1.f);

	int32 NrOfBots = 0;
	for (FOverlapResult Result : Overlaps)
	{
		ASTrackerBot* Bot = Cast<ASTrackerBot>(Result.GetActor());
		
		if (Bot && Bot != this)
		{
			NrOfBots++;
		}	
	}
	const int32 MaxPowerLevel = 4;

	PowerLevel = FMath::Clamp(NrOfBots, 0, MaxPowerLevel);

	if (MatInst == nullptr)
	{
		MatInst = MeshComp->CreateAndSetMaterialInstanceDynamicFromMaterial(0, MeshComp->GetMaterial(0));
	}
	if (MatInst)
	{
		float Alpha = PowerLevel / (float)MaxPowerLevel;
		MatInst->SetScalarParameterValue("PowerLevelAlpha", Alpha);
	}

	DrawDebugString(GetWorld(), FVector(0, 0, 0), FString::FromInt(PowerLevel), this, FColor::White, 1.f, true);
}

// Called every frame
void ASTrackerBot::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	if (Role == ROLE_Authority)
	{
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
	}

}

void ASTrackerBot::NotifyActorBeginOverlap(AActor* OtherActor)
{
	ASCharacter* MyPawn = Cast<ASCharacter>(OtherActor);
	if (!bOverlaped && MyPawn)
	{
		bOverlaped = true;
		if(Role == ROLE_Authority)
		{
			GetWorldTimerManager().SetTimer(TimerHandle_SelfDamage, this, &ASTrackerBot::DamageSelf,
				SelfDamageInterval, true, 0.0f);
		}
	
		UGameplayStatics::SpawnSoundAttached(SelfDestructSound, RootComponent);
	}
}

