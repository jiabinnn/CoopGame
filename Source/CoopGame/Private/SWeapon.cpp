// Fill out your copyright notice in the Description page of Project Settings.

#include "SWeapon.h"
#include "Components/SkeletalMeshComponent.h"
#include "DrawDebugHelpers.h"
#include "Kismet/GameplayStatics.h"
#include "Particles/ParticleSystem.h"
#include "SCharacter.h"
#include "Particles/ParticleSystemComponent.h"
#include "CoopGame.h"
#include "PhysicalMaterials/PhysicalMaterial.h"
#include "TimerManager.h"
#include <UnrealNetwork.h>
static int32 DebugWeaponDrawing = 0;
FAutoConsoleVariableRef CVARDebugWeaponDrawing(
	TEXT("COOP.DebugWeapons"), 
	DebugWeaponDrawing, 
	TEXT("Draw Debug Lines for Weapons"), 
	ECVF_Cheat
);


// Sets default values
ASWeapon::ASWeapon()
{

	MeshComp = CreateDefaultSubobject<USkeletalMeshComponent>(TEXT("MeshComp"));
	RootComponent = MeshComp;

	MuzzleSocketName = "MuzzleSocket";
	TracerTargetName = "BeamEnd";

	BaseDamage = 20.f;
	RateOfFire = 600.f;

	SetReplicates(true);

	NetUpdateFrequency = 66.f;
	MinNetUpdateFrequency = 33.f;

	//TotalBulletsNum = 100;
	//MagazineNum = 30;
	//CurrentBulletsNum = MagazineNum;
}

void ASWeapon::BeginPlay()
{
	Super::BeginPlay();
	TimeBetweenShots = 60 / RateOfFire;
}



void ASWeapon::Fire()
{

	if (Role < ROLE_Authority)
	{
		ServerFire();

	}

// 	if (CurrentBulletsNum <= 0)
// 	{
// 		return;
// 	}
// 	CurrentBulletsNum--;

// 	OnBulletChanged();

	//Trace the world, from pawn eyes to crosshair location
	AActor* MyOwner = GetOwner();
	if (MyOwner)
	{
		FVector EyeLocation;
		FRotator EyeRotation;
		MyOwner->GetActorEyesViewPoint(EyeLocation, EyeRotation);
		FVector ShotDirection = EyeRotation.Vector();

		FCollisionQueryParams QueryParams;
		QueryParams.AddIgnoredActor(MyOwner);
		QueryParams.AddIgnoredActor(this);
		QueryParams.bTraceComplex = true;
		QueryParams.bReturnPhysicalMaterial = true;

		FVector EndPoint = EyeLocation + (EyeRotation.Vector() * 10000);

		FVector TracerEndPoint = EndPoint;

		FVector MuzzleLocation = MeshComp->GetSocketLocation(MuzzleSocketName);

		EPhysicalSurface SurfaceType = SurfaceType_Default;

		FHitResult Hit;
		if (GetWorld()->LineTraceSingleByChannel(Hit, EyeLocation, EndPoint, COLLISION_WEAPON, QueryParams))
		{
			//Block Hit! Process damage
			AActor* HitActor = Hit.GetActor();

			SurfaceType = UPhysicalMaterial::DetermineSurfaceType(Hit.PhysMaterial.Get());

			float ActualDamage = BaseDamage;
			if (SurfaceType == SURFACE_FLESHVULNEABLE)
			{
				ActualDamage *= 4.f;
			}
		

			UGameplayStatics::ApplyPointDamage(HitActor, ActualDamage, ShotDirection, Hit, MyOwner->GetInstigatorController(), this, DamageType);
			
			PlayImpactEffects(SurfaceType, Hit.ImpactPoint);

			TracerEndPoint = Hit.ImpactPoint;
			HitScanTrace.SurfaceType = SurfaceType;
		}

		if (DebugWeaponDrawing > 0)
		{
			DrawDebugLine(GetWorld(), EyeLocation, EndPoint, FColor::Red, false, 1.f, 0, 1.f);
		}

		PlayFireEffects(TracerEndPoint);

		if (Role == ROLE_Authority)
		{
			HitScanTrace.TraceTo = TracerEndPoint;
		}

	}
	LastFireTime = GetWorld()->TimeSeconds;
}


void ASWeapon::StartFire()
{
	float FirstDelay = FMath::Max(LastFireTime + TimeBetweenShots - GetWorld()->TimeSeconds, 0.f);

	GetWorldTimerManager().SetTimer(FireTimerHandle,this, &ASWeapon::Fire, TimeBetweenShots, true, FirstDelay);
}

void ASWeapon::StopFire()
{
	GetWorldTimerManager().ClearTimer(FireTimerHandle);
}

void ASWeapon::OnRep_HitScanTrace()
{

	PlayFireEffects(HitScanTrace.TraceTo);
	PlayImpactEffects(HitScanTrace.SurfaceType, HitScanTrace.TraceTo);
}

void ASWeapon::PlayImpactEffects(EPhysicalSurface SurfaceType, FVector ImpactPoint)
{
	UParticleSystem* SelectedEffect;

	switch (SurfaceType)
	{
	case SURFACE_FLESHDEFAULT:
		SelectedEffect = FleshImpactEffect;
		break;
	case SURFACE_FLESHVULNEABLE:
		SelectedEffect = FleshImpactEffect;
		break;
	default:
		SelectedEffect = DefaultImpactEffect;
		break;
	}

	if (SelectedEffect)
	{
		FVector MuzzleLocation = MeshComp->GetSocketLocation(MuzzleSocketName);

		FVector ShotDirection = ImpactPoint - MuzzleLocation;
		ShotDirection.Normalize();

		UGameplayStatics::SpawnEmitterAtLocation(GetWorld(), SelectedEffect, ImpactPoint, ShotDirection.Rotation());
	}

}

void ASWeapon::PlayFireEffects(FVector TracerEndPoint)
{
	if (MuzzleEffect)
	{
		UGameplayStatics::SpawnEmitterAttached(MuzzleEffect, MeshComp, MuzzleSocketName);
	}
	if (TracerEffect)
	{
		FVector MuzzleLocation = MeshComp->GetSocketLocation(MuzzleSocketName);

		UParticleSystemComponent* TracerComp = UGameplayStatics::SpawnEmitterAtLocation(GetWorld(), TracerEffect, MuzzleLocation);
		if (TracerComp)
		{
			TracerComp->SetVectorParameter(TracerTargetName, TracerEndPoint);
		}
	}

	APawn* MyOwner = Cast<APawn>(GetOwner());
	if (MyOwner)
	{
		APlayerController* PC = Cast<APlayerController>(MyOwner->GetController());
		if (PC)
		{
			PC->ClientPlayCameraShake(FireCamShake);
		}
	}


}

void ASWeapon::ServerFire_Implementation()
{
	Fire();
}

bool ASWeapon::ServerFire_Validate()
{
	return true;
}

void ASWeapon::GetLifetimeReplicatedProps(TArray< FLifetimeProperty > & OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME_CONDITION(ASWeapon, HitScanTrace, COND_SkipOwner);

}



// void ASWeapon::ReloadBullet_Implementation()
// {
// 	if (TotalBulletsNum > MagazineNum - CurrentBulletsNum)
// 	{
// 		TotalBulletsNum -= MagazineNum - CurrentBulletsNum;
// 		CurrentBulletsNum = MagazineNum;
// 	}
// 	else
// 	{
// 		CurrentBulletsNum += TotalBulletsNum;
// 		TotalBulletsNum = 0;
// 
// 	}		
// 
// 
// 
// 	OnBulletChanged();
// 	//if (TotalBulletsNum < MagazineNum) 
// 	//{
// 	//	CurrentBulletsNum = TotalBulletsNum; 
// 	//	TotalBulletsNum = 0;
// 	//}
// 	//else
// 	//{
// 	//	TotalBulletsNum -= MagazineNum;
// 	//	CurrentBulletsNum = MagazineNum;
// 	//}
// }
// 



