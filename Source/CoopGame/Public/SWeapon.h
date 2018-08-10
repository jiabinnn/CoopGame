// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "SWeapon.generated.h"
class USkeletalMeshComponent;
class UDamageType;
class UParticleSystem;

UCLASS()
class COOPGAME_API ASWeapon : public AActor
{
	GENERATED_BODY()
	
public:	
	// Sets default values for this actor's properties
	ASWeapon();

	UFUNCTION(BlueprintCallable, Category = "Weapon")
	virtual void Fire();

	void StartFire();

	void StopFire();


protected:

	virtual void BeginPlay() override;

	UPROPERTY(VisibleAnywhere,BlueprintReadOnly,Category = "Components")
	USkeletalMeshComponent* MeshComp;

	void PlayFireEffects(FVector TracerEndPoint);

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Weapon")
	TSubclassOf<UDamageType> DamageType;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Weapon")
	UParticleSystem* MuzzleEffect;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Weapon")
	UParticleSystem* DefaultImpactEffect;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Weapon")
	UParticleSystem* FleshImpactEffect;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Weapon")
	UParticleSystem* TracerEffect;

	UPROPERTY(VisibleDefaultsOnly, BlueprintReadOnly, Category = "Weapon")
	FName MuzzleSocketName;

	UPROPERTY(VisibleDefaultsOnly, BlueprintReadOnly, Category = "Weapon")
	FName TracerTargetName;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Weapon")
	float BaseDamage;

	UPROPERTY(EditDefaultsOnly,Category = "Weapon")
	TSubclassOf<UCameraShake> FireCamShake;

	FTimerHandle FireTimerHandle;

	/* RPM - Bullets per minute fired by weapon */
	UPROPERTY(EditDefaultsOnly, Category = "Weapon")
	float RateOfFire;

	/* Derived from RateOfFire */
	float TimeBetweenShots;

	float LastFireTime;

protected:

// 	UPROPERTY(EditDefaultsOnly,BlueprintReadOnly,Category = "Weapon")
// 	int TotalBulletsNum;
// 
// 	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Weapon")
// 	int MagazineNum;
// 
// 	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Weapon")
// 	int CurrentBulletsNum;
// 
// 	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Weapon")
// 	void ReloadBullet();
// 
// 	UFUNCTION(BlueprintImplementableEvent,Category = "Weapon")
// 	void OnBulletChanged();
};
