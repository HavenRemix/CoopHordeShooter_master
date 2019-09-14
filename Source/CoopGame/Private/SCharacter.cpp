// Fill out your copyright notice in the Description page of Project Settings.

#include "SCharacter.h"
#include "Camera/CameraComponent.h"
#include "GameFramework/SpringArmComponent.h"
#include "GameFramework/PawnMovementComponent.h"
#include "Components/CapsuleComponent.h"
#include "CoopGame.h"
#include "SHealthComponent.h"
#include "SWeapon.h"
#include "Net/UnrealNetwork.h"
#include "GameFramework/CharacterMovementComponent.h"



// Sets default values
ASCharacter::ASCharacter()
{
 	// Set this character to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;

//Spring Arm

	SpringArmComp = CreateDefaultSubobject<USpringArmComponent>(TEXT("SpringArmComp"));
	SpringArmComp->bUsePawnControlRotation = true;
	SpringArmComp->SetupAttachment(RootComponent);

//Movement Comp

	GetCharacterMovement()->MaxWalkSpeed = DefaultMovementSpeed;

	//Make crouch true in the default movement so we can crouch using the default function
	GetMovementComponent()->GetNavAgentPropertiesRef().bCanCrouch = true;

	//Ignore the collision capsule so weapons hit the mesh and not the capsule
	GetCapsuleComponent()->SetCollisionResponseToChannel(COLLISION_WEAPON, ECR_Ignore);

//Health Comp

	HealthComp = CreateDefaultSubobject<USHealthComponent>(TEXT("HealthComp"));

//Camera Comp

	CameraComp = CreateDefaultSubobject<UCameraComponent>(TEXT("CameraComp"));
	CameraComp->SetupAttachment(SpringArmComp);

//Weapon Variables

	ZoomedFOV = 40.0f;
	ZoomInterpSpeed = 20;

	WeaponAttachSocketName = "WeaponSocket";

	//Set default variables
	DefaultMovementSpeed = 350;
	SprintingMovementSpeed = 850;
	CrouchedMovementSpeed = 200;

	bReloading = false;
}

// Called when the game starts or when spawned
void ASCharacter::BeginPlay()
{
	Super::BeginPlay();
	
	DefaultFOV = CameraComp->FieldOfView;
	HealthComp->OnHealthChanged.AddDynamic(this, &ASCharacter::OnHealthChanged);

	if (Role == ROLE_Authority)
	{
		// Spawn a default weapon
		FActorSpawnParameters SpawnParams;
		SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

		CurrentWeapon = GetWorld()->SpawnActor<ASWeapon>(StarterWeaponClass, FVector::ZeroVector, FRotator::ZeroRotator, SpawnParams);
		if (CurrentWeapon)
		{
			CurrentWeapon->SetOwner(this);
			CurrentWeapon->AttachToComponent(GetMesh(), FAttachmentTransformRules::SnapToTargetNotIncludingScale, WeaponAttachSocketName);
		}
	}
}

// Called every frame
void ASCharacter::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	float TargetFOV = bWantsToZoom ? ZoomedFOV : DefaultFOV;
	float NewFOV = FMath::FInterpTo(CameraComp->FieldOfView, TargetFOV, DeltaTime, ZoomInterpSpeed);

	CameraComp->SetFieldOfView(NewFOV);
}

// Called to bind functionality to input
void ASCharacter::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);

	PlayerInputComponent->BindAxis("MoveForward", this, &ASCharacter::MoveForward);
	PlayerInputComponent->BindAxis("MoveRight", this, &ASCharacter::MoveRight);

	PlayerInputComponent->BindAxis("LookUp", this, &ASCharacter::LookUp);
	PlayerInputComponent->BindAxis("Turn", this, &ASCharacter::Turn);

	PlayerInputComponent->BindAction("Reload", IE_Pressed, this, &ASCharacter::Reload);
	PlayerInputComponent->BindAction("Reload", IE_Released, this, &ASCharacter::EndReload);

	PlayerInputComponent->BindAction("Crouch", IE_Pressed, this, &ASCharacter::BeginCrouch);
	PlayerInputComponent->BindAction("Crouch", IE_Released, this, &ASCharacter::EndCrouch);

	PlayerInputComponent->BindAction("Sprint", IE_Pressed, this, &ASCharacter::BeginSprint);
	PlayerInputComponent->BindAction("Sprint", IE_Released, this, &ASCharacter::EndSprint);

	PlayerInputComponent->BindAction("Zoom", IE_Pressed, this, &ASCharacter::BeginZoom);
	PlayerInputComponent->BindAction("Zoom", IE_Released, this, &ASCharacter::EndZoom);

	PlayerInputComponent->BindAction("Fire", IE_Pressed, this, &ASCharacter::StartFire);
	PlayerInputComponent->BindAction("Fire", IE_Released, this, &ASCharacter::StopFire);

	// CHALLENGE CODE
	PlayerInputComponent->BindAction("Jump", IE_Pressed, this, &ACharacter::Jump);
}

// ------- INPUT ------- \\

void ASCharacter::MoveForward(float Value)
{
	AddMovementInput(GetActorForwardVector() * Value);

	if (Value > 0.0f)
	{
		CurrentWeapon->IsMoving = true;
	}
	else {
		CurrentWeapon->IsMoving = false;
	}
}


void ASCharacter::MoveRight(float Value)
{
	AddMovementInput(GetActorRightVector() * Value);

	if (Value > 0.0f)
	{
		CurrentWeapon->IsMoving = true;
	}
	else {
		CurrentWeapon->IsMoving = false;
	}
}


void ASCharacter::LookUp(float value)
{
	//This is the since that is value times the set mouse Y sens by the player
	float MainSens = value * MouseYSens;

	//This is the sens for when your targeting
	float NewSensitivity = MainSens * MouseTargetingSens;

	if (bWantsToZoom)
	{
		AddControllerPitchInput(NewSensitivity);
	}
	else {
		AddControllerPitchInput(MainSens);
	}
}


void ASCharacter::Turn(float value)
{
	//This is the since that is value times the set mouse X sens by the player
	float MainSens = value * MouseXSens;

	//This is the sens for when your targeting
	float NewSensitivity = MainSens * MouseTargetingSens;

	if (bWantsToZoom)
	{
		AddControllerYawInput(NewSensitivity);
	}
	else {
		AddControllerYawInput(MainSens);
	}
}


void ASCharacter::BeginCrouch()
{
	Crouch();
}


void ASCharacter::EndCrouch()
{
	UnCrouch();
}


void ASCharacter::BeginSprint()
{
	GetCharacterMovement()->MaxWalkSpeed = SprintingMovementSpeed;
}


void ASCharacter::EndSprint()
{
	GetCharacterMovement()->MaxWalkSpeed = DefaultMovementSpeed;
}


void ASCharacter::BeginZoom()
{
	bWantsToZoom = true;
	CurrentWeapon->IsAiming = true;
}


void ASCharacter::EndZoom()
{
	bWantsToZoom = false;
	CurrentWeapon->IsAiming = false;
}


void ASCharacter::StartFire()
{
	if (CurrentWeapon)
	{
		CurrentWeapon->StartFire();
	}
}


void ASCharacter::StopFire()
{
	if (CurrentWeapon)
	{
		CurrentWeapon->StopFire();
	}
}


void ASCharacter::Reload()
{
	if (CurrentWeapon)
	{
		CurrentWeapon->ReloadWeapon();
		bReloading = true;
	}
}

void ASCharacter::EndReload()
{
	bReloading = false;
}

// ------- FUNCTION ------- \\

void ASCharacter::OnHealthChanged(USHealthComponent* OwningHealthComp, float Health, float HealthDelta, const class UDamageType* DamageType,
	class AController* InstigatedBy, AActor* DamageCauser)
{
	if (Health <= 0.0f && !bDied)
	{
		// Die!
		bDied = true;

		//Stop movement immediately
		GetMovementComponent()->StopMovementImmediately();

		//Set it so there is no collision
		GetCapsuleComponent()->SetCollisionEnabled(ECollisionEnabled::NoCollision);

		//Detach from controller so it cant have input or do anything
		DetachFromControllerPendingDestroy();

		//Set lifespan
		SetLifeSpan(10.0f);
	}
}


FVector ASCharacter::GetPawnViewLocation() const
{
	if (CameraComp)
	{
		//Change the eyes view location to the camera component location
		return CameraComp->GetComponentLocation();
	}

	//If camera component doesn't exist for some reason then it goes back to the default function
	return Super::GetPawnViewLocation();
}

// ------- ONLINE ------- \\

void ASCharacter::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(ASCharacter, CurrentWeapon);
	DOREPLIFETIME(ASCharacter, bDied);
}