// Fill out your copyright notice in the Description page of Project Settings.

#include "SWeapon.h"
#include "DrawDebugHelpers.h"
#include "Kismet/GameplayStatics.h"
#include "Particles/ParticleSystem.h"
#include "Components/SkeletalMeshComponent.h"
#include "Particles/ParticleSystemComponent.h"
#include "PhysicalMaterials/PhysicalMaterial.h"
#include "CoopGame.h"
#include "TimerManager.h"
#include "Net/UnrealNetwork.h"

static int32 DebugWeaponDrawing = 0;
FAutoConsoleVariableRef CVARDebugWeaponDrawing(
	TEXT("COOP.DebugWeapons"), 
	DebugWeaponDrawing, 
	TEXT("Draw Debug Lines for Weapons"), 
	ECVF_Cheat);


// Sets default values
ASWeapon::ASWeapon()
{
	MeshComp = CreateDefaultSubobject<USkeletalMeshComponent>(TEXT("MeshComp"));
	RootComponent = MeshComp;

	MuzzleSocketName = "MuzzleSocket";
	TracerTargetName = "Target";

	BaseDamage = 20.0f;
	HeadshotMultiplier = 2.0f;

	BulletSpread = 5.0f;
	AimingBulletSpread = 0.0f;
	CrouchedBulletSpread = 2.0f;
	MovingBulletSpread = 7.0f;

	RateOfFire = 600;

	SetReplicates(true);

	NetUpdateFrequency = 66.0f;
	MinNetUpdateFrequency = 33.0f;

	MaxAmmo = 120;
	CurrentAmmo = 30;
	ClipSize = 30;
}


void ASWeapon::BeginPlay()
{
	Super::BeginPlay();

	//Set rate of fire relative to time
	TimeBetweenShots = 60 / RateOfFire;
}

// ------- FUNCTION ------- \\

void ASWeapon::Fire()
{
	// Trace the world, from pawn eyes to crosshair location

	if (Role < ROLE_Authority)
	{
		//Call server fire if we are not the server
		ServerFire();
	}

	AActor* MyOwner = GetOwner();
	if (MyOwner)
	{
		//Make sure that we have ammo before we fire
		if (CurrentAmmo > 0)
		{
			//Name variables for location
			FVector EyeLocation;
			FRotator EyeRotation;
			MyOwner->GetActorEyesViewPoint(EyeLocation, EyeRotation);

			FVector ShotDirection = EyeRotation.Vector();

			// Bullet Spread
			float HalfRad;

			//Set the bullet spread acccording to the action that the character is doing
			if (IsAiming)
			{
				HalfRad = FMath::DegreesToRadians(AimingBulletSpread);
			}
			else if(IsCrouched) {
				HalfRad = FMath::DegreesToRadians(CrouchedBulletSpread);
			}
			else if(IsMoving) {
				HalfRad = FMath::DegreesToRadians(MovingBulletSpread);
			}
			else {
				HalfRad = FMath::DegreesToRadians(BulletSpread);
			}
			ShotDirection = FMath::VRandCone(ShotDirection, HalfRad, HalfRad);

			FVector TraceEnd = EyeLocation + (ShotDirection * 10000);

			FCollisionQueryParams QueryParams;
			QueryParams.AddIgnoredActor(MyOwner);
			QueryParams.AddIgnoredActor(this);
			QueryParams.bTraceComplex = true;
			QueryParams.bReturnPhysicalMaterial = true;

			// Particle "Target" parameter
			FVector TracerEndPoint = TraceEnd;

			EPhysicalSurface SurfaceType = SurfaceType_Default;

			FHitResult Hit;
			//Only run if its a blocking hit
			if (GetWorld()->LineTraceSingleByChannel(Hit, EyeLocation, TraceEnd, COLLISION_WEAPON, QueryParams))
			{
				// Blocking hit! Process damage
				AActor* HitActor = Hit.GetActor();

				//Get the surface type that we hit
				SurfaceType = UPhysicalMaterial::DetermineSurfaceType(Hit.PhysMaterial.Get());

				//This is the actuall damage that we did
				float ActualDamage = BaseDamage;
				//Only run if we hit a flesh vunerable surface type
				if (SurfaceType == SURFACE_FLESHVULNERABLE)
				{
					//For headshots multiply the actuall damage by the headshot multiplier
					ActualDamage *= HeadshotMultiplier;
				}

				//Apply damage to the object that we hit
				UGameplayStatics::ApplyPointDamage(HitActor, ActualDamage, ShotDirection, Hit, MyOwner->GetInstigatorController(), MyOwner, DamageType);

				//Play effects for mpact
				PlayImpactEffects(SurfaceType, Hit.ImpactPoint);

				//Set tracer end point to where we hit the actor so the tracer effect doesn't keep on going on
				TracerEndPoint = Hit.ImpactPoint;

			}

			if (DebugWeaponDrawing > 0)
			{
				//If debugging is enabled by the console then draw debug lines for the weapon
				DrawDebugLine(GetWorld(), EyeLocation, TraceEnd, FColor::White, false, 1.0f, 0, 1.0f);
			}

			//Play the effects for firing the weapon
			PlayFireEffects(TracerEndPoint);

			//Only run if we are the server
			if (Role == ROLE_Authority)
			{
				HitScanTrace.TraceTo = TracerEndPoint;
				HitScanTrace.SurfaceType = SurfaceType;
			}

			LastFireTime = GetWorld()->TimeSeconds;

			//Reduce ammo by once every time we fire
			CurrentAmmo--;
		}
	}
}


void ASWeapon::OnRep_HitScanTrace()
{
	// Play cosmetic FX
	PlayFireEffects(HitScanTrace.TraceTo);
	PlayImpactEffects(HitScanTrace.SurfaceType, HitScanTrace.TraceTo);
}


void ASWeapon::ServerFire_Implementation()
{
	Fire();
}


bool ASWeapon::ServerFire_Validate()
{
	//Make anti cheat code here
	return true;
}


void ASWeapon::StartFire()
{
	float FirstDelay = FMath::Max(LastFireTime + TimeBetweenShots - GetWorld()->TimeSeconds, 0.0f);

	GetWorldTimerManager().SetTimer(TimerHandle_TimeBetweenShots, this, &ASWeapon::Fire, TimeBetweenShots, true, FirstDelay);
}


void ASWeapon::StopFire()
{
	GetWorldTimerManager().ClearTimer(TimerHandle_TimeBetweenShots);
}


void ASWeapon::ReloadWeapon()
{
	AActor* MyOwner = GetOwner();
	if (MyOwner)
	{
		CurrentAmmo = ClipSize;
	}
}


void ASWeapon::PlayFireEffects(FVector TraceEnd)
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
			TracerComp->SetVectorParameter(TracerTargetName, TraceEnd);
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


void ASWeapon::PlayImpactEffects(EPhysicalSurface SurfaceType, FVector ImpactPoint)
{
	UParticleSystem* SelectedEffect = nullptr;
	switch (SurfaceType)
	{
	case SURFACE_FLESHDEFAULT:
	case SURFACE_FLESHVULNERABLE:
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


void ASWeapon::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME_CONDITION(ASWeapon, HitScanTrace, COND_SkipOwner);
}
