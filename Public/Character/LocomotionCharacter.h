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
	void Dodge(float Forward, float Right); // entry point, called on the owning client
		
	bool IsDodging() const; // true while any dodge montage is playing
	
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
	
	UPROPERTY(BlueprintReadOnly, Category = "L0CO|Strafing")
	float MovementOffsetYaw;
	
	UPROPERTY(BlueprintReadOnly, Category = "LOCO|TurnInPlace")
	ETurningInPlace TurningStatus = ETurningInPlace::NotTurning;
	
	UFUNCTION(Server, Reliable) // runs on the server when called from a client
	void Server_Dodge(float Forward, float Right); // server-side dodge request
	
	UFUNCTION(NetMulticast, Reliable) // runs on the server and every client
	void Multicast_Dodge(UAnimMontage* Montage); // NEW: plays the dodge on other players' copies Int32?
	
	UPROPERTY(EditDefaultsOnly, Category = "CB|Dodge") // assign in the Blueprint defaults
	TObjectPtr<UAnimMontage> DodgeForwardMontage; // root motion forward roll
	UPROPERTY(EditDefaultsOnly, Category = "CB|Dodge") // NEW
	TObjectPtr<UAnimMontage> DodgeBackMontage; // root motion back roll
	UPROPERTY(EditDefaultsOnly, Category = "CB|Dodge")
	TObjectPtr<UAnimMontage> DodgeLeftMontage; // root motion left roll
	UPROPERTY(EditDefaultsOnly, Category = "CB|Dodge") // NEW
	TObjectPtr<UAnimMontage> DodgeRightMontage; // NEW: root motion right roll
	
private:
	
	void Input_Aim_Pressed();
	void Input_Aim_Released();
	void CalculateTurnInPlaceParameters(float DeltaTime);
	void TurnInPlace(float DeltaTime);
	
	FRotator StartingAimRotation;
	float InterpAO_Yaw;
	
	UAnimMontage* GetDodgeMontage(float Forward, float Right) const; // picks the montage from the input
	
	UPROPERTY(EditAnywhere, Category = "LOCO|Input")
	TObjectPtr<UInputAction> AimWeaponAction;
};