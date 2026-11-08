// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Data/Enums.h"
#include "GameFramework/Character.h"
#include "Logging/LogMacros.h"
#include "LocomotionCharacter.generated.h"

enum class ETurningInPlace : uint8;
class UCombatComponent;
class USpringArmComponent;
class UCameraComponent;
class UInputAction;
struct FInputActionValue;


UCLASS(abstract)
class ALocomotionCharacter : public ACharacter
{
	GENERATED_BODY()
	
public:

	/** Constructor */
	ALocomotionCharacter();	
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
	
	virtual void Tick(float DeltaTime) override;
	virtual void PossessedBy(AController* EventController) override;
	virtual void SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent) override;
	virtual void OnRep_PlayerState() override;
	
	virtual void BeginPlay() override;
	
	UFUNCTION(BlueprintCallable)
	FRotator GetFixedAimRotation() const;
	
	
protected:
	/** Camera boom positioning the camera behind the character */

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "LOCO|Camera")
	TObjectPtr<USpringArmComponent> CameraBoom;;
	
	/** Follow camera */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "LOCO|Camera")
	TObjectPtr<UCameraComponent> FollowCamera;	
	
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "LOCO|Combat")
	TObjectPtr<UCombatComponent> Combat;
	
	UFUNCTION(BlueprintImplementableEvent) //only done locally in blueprint
	void OnAim(bool bIsAiming);
	
	UPROPERTY(BlueprintReadOnly, Category = "LOCO|TurnInPlace")
	float AO_Yaw; //stores AO_Yaw
	
	UPROPERTY(BlueprintReadOnly, Category = "FPS|Strafing")
	float MovementOffsetYaw;
	
	UPROPERTY(BlueprintReadOnly, Category = "LOCO|TurnInPlace")
	ETurningInPlace TurningStatus = ETurningInPlace::NotTurning;
	
private:
	
	void Input_Aim_Pressed();
	void Input_Aim_Released();
	void CalculateTurnInPlaceParameters(float DeltaTime);
	void TurnInPlace(float DeltaTime);
	
	FRotator StartingAimRotation;
	float InterpAO_Yaw;
	
	UPROPERTY(EditAnywhere, Category = "LOCO|Input")
	TObjectPtr<UInputAction> AimWeaponAction;
};