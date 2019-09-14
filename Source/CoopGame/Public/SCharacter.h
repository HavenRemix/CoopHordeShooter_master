// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "SCharacter.generated.h"

class UCameraComponent;
class USpringArmComponent;
class ASWeapon;
class USHealthComponent;

UCLASS()
class COOPGAME_API ASCharacter : public ACharacter
{
	GENERATED_BODY()

public:
	// Sets default values for this character's properties
	ASCharacter();

protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

// ------- INPUT ------- \\

	void MoveForward(float Value);

	void MoveRight(float Value);

	void LookUp(float value);

	void Turn(float value);

	void BeginCrouch();

	void EndCrouch();

	void BeginSprint();

	void EndSprint();

	void BeginZoom();

	void EndZoom();

	void Reload();

	void EndReload();

// ------- COMPONENTS ------- \\

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	UCameraComponent* CameraComp;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	USpringArmComponent* SpringArmComp;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	USHealthComponent* HealthComp;

// ------- FUNCTION ------- \\

	UFUNCTION()
	void OnHealthChanged(USHealthComponent* OwningHealthComp, float Health, float HealthDelta, const class UDamageType* DamageType, class AController* InstigatedBy, AActor* DamageCauser);

// ------- VARIABLES ------- \\

//Bool

	bool bWantsToZoom;

	/* Pawn died previously */
	UPROPERTY(Replicated, BlueprintReadOnly, Category = "Player")
	bool bDied;

//Float

	UPROPERTY(BlueprintReadWrite, Category = "Sensitivity")
	float MouseYSens;

	UPROPERTY(BlueprintReadWrite, Category = "Sensitivity")
	float MouseXSens;

	UPROPERTY(BlueprintReadWrite, Category = "Sensitivity")
	float MouseTargetingSens;

	UPROPERTY(EditDefaultsOnly, Category = "Movement")
	float DefaultMovementSpeed;

	UPROPERTY(EditDefaultsOnly, Category = "Movement")
	float SprintingMovementSpeed;

	UPROPERTY(EditDefaultsOnly, Category = "Movement")
	float CrouchedMovementSpeed;

	UPROPERTY(EditDefaultsOnly, Category = "Player")
	float ZoomedFOV;

	UPROPERTY(EditDefaultsOnly, Category = "Player", meta = (ClampMin = 0.1, ClampMax = 100))
	float ZoomInterpSpeed;

	/* Default FOV set during begin play */
	float DefaultFOV;


// ------- WEAPON ------- \\

	UPROPERTY(Replicated)
		ASWeapon* CurrentWeapon;

	UPROPERTY(EditDefaultsOnly, Category = "Player")
	TSubclassOf<ASWeapon> StarterWeaponClass;

	UPROPERTY(VisibleDefaultsOnly, Category = "Player")
	FName WeaponAttachSocketName;

public:	
	// Called every frame
	virtual void Tick(float DeltaTime) override;

	// Called to bind functionality to input
	virtual void SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent) override;

// ------- FUNCTIONS ------- \\

	virtual FVector GetPawnViewLocation() const override;

// ------- INPUT ------- \\

	UFUNCTION(BlueprintCallable, Category = "Player")
	void StartFire();

	UFUNCTION(BlueprintCallable, Category = "Player")
	void StopFire();

// ------- VARIABLES ------- \\

//Bool

	UPROPERTY(BlueprintReadOnly, Category = "Player")
	bool bReloading;
};
