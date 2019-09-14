// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "SWeapon.generated.h"

class USkeletalMeshComponent;
class UDamageType;
class UParticleSystem;

// Contains information of a single hitscan weapon linetrace
USTRUCT()
struct FHitScanTrace
{
	GENERATED_BODY()

public:

	UPROPERTY()
	TEnumAsByte<EPhysicalSurface> SurfaceType;

	UPROPERTY()
	FVector_NetQuantize TraceTo;
};


UCLASS()
class COOPGAME_API ASWeapon : public AActor
{
	GENERATED_BODY()
	
public:	
	// Sets default values for this actor's properties
	ASWeapon();

protected:

	virtual void BeginPlay() override;

// ------- COMPONENTS ------- \\

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	USkeletalMeshComponent* MeshComp;

// ------- FUNCTIONS ------- \\

	void PlayFireEffects(FVector TraceEnd);

	void PlayImpactEffects(EPhysicalSurface SurfaceType, FVector ImpactPoint);

	void Fire();

	UFUNCTION(Server, Reliable, WithValidation)
	void ServerFire();

	UFUNCTION()
	void OnRep_HitScanTrace();

// ------- VARIABLES ------- \\

//Damage Type

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Weapon")
	TSubclassOf<UDamageType> DamageType;

//FName

	UPROPERTY(VisibleDefaultsOnly, BlueprintReadOnly, Category = "Weapon")
	FName MuzzleSocketName;

	UPROPERTY(VisibleDefaultsOnly, BlueprintReadOnly, Category = "Weapon")
	FName TracerTargetName;

//Particle System

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Weapon")
	UParticleSystem* MuzzleEffect;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Weapon")
	UParticleSystem* DefaultImpactEffect;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Weapon")
	UParticleSystem* FleshImpactEffect;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Weapon")
	UParticleSystem* TracerEffect;

//Camera Shake

	UPROPERTY(EditDefaultsOnly, Category = "Weapon")
	TSubclassOf<UCameraShake> FireCamShake;

//Float

	UPROPERTY(EditDefaultsOnly, Category = "Damage")
	float BaseDamage;

	UPROPERTY(EditDefaultsOnly, Category = "Damage")
	float HeadshotMultiplier;

	/* RPM - Bullets per minute fired by weapon */
	UPROPERTY(EditDefaultsOnly, Category = "Weapon")
	float RateOfFire;

	/* Bullet Spread in Degrees */
	UPROPERTY(EditDefaultsOnly, Category = "Weapon", meta = (ClampMin = 0.0f))
	float BulletSpread;

	/* Bullet Spread in Degrees */
	UPROPERTY(EditDefaultsOnly, Category = "Weapon", meta = (ClampMin = 0.0f))
	float AimingBulletSpread;

	/* Bullet Spread in Degrees */
	UPROPERTY(EditDefaultsOnly, Category = "Weapon", meta = (ClampMin = 0.0f))
	float CrouchedBulletSpread;

	/* Bullet Spread in Degrees */
	UPROPERTY(EditDefaultsOnly, Category = "Weapon", meta = (ClampMin = 0.0f))
	float MovingBulletSpread;
	
	// Derived from RateOfFire
	float TimeBetweenShots;

	float LastFireTime;

//Time Handle

	FTimerHandle TimerHandle_TimeBetweenShots;

//Hit Scan Trace

	UPROPERTY(ReplicatedUsing=OnRep_HitScanTrace)
	FHitScanTrace HitScanTrace;

public:	

// ------- FUNCTION ------- \\

	void StartFire();

	void StopFire();

	void ReloadWeapon();

// ------- VARIABLES ------- \\

//Float

	UPROPERTY(BlueprintReadWrite, Category = "Ammo")
	float MaxAmmo;

	UPROPERTY(BlueprintReadWrite, Category = "Ammo")
	float CurrentAmmo;

	UPROPERTY(BlueprintReadWrite, Category = "Ammo")
	float ClipSize;

//Bool

	bool IsAiming;

	bool IsCrouched;

	bool IsMoving;
	
};
