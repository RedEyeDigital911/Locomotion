// Copyright Red Eye Digital, LLC, All Rights Reserved

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerController.h"
#include "LocomotionPlayerController.generated.h"

class UInputAction;
class UInputMappingContext;
struct FInputActionValue;

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FPlayerStateReplicated);

UCLASS(abstract)
class ALocomotionPlayerController : public APlayerController
{
	GENERATED_BODY()
	
public:
	ALocomotionPlayerController();
	
	FPlayerStateReplicated OnPlayerStateReplicated;
	virtual void OnRep_PlayerState() override;

	bool bPawnAlive;
	
protected:	
	virtual void BeginPlay() override;
	virtual void SetupInputComponent() override;
	virtual void OnPossess(APawn* InPawn) override;
	
private:
	UPROPERTY( EditAnywhere, Category = "LOCO|Input")
	TObjectPtr<UInputMappingContext> IMC_Player;

	UPROPERTY( EditAnywhere, Category = "LOCO|Input")
	TObjectPtr<UInputAction> MoveAction;

	UPROPERTY( EditAnywhere, Category = "LOCO|Input")
	TObjectPtr<UInputAction> LookAction;

	UPROPERTY( EditAnywhere, Category = "LOCO|Input")
	TObjectPtr<UInputAction> CrouchAction;

	UPROPERTY( EditAnywhere, Category = "LOCO|Input")
	TObjectPtr<UInputAction> JumpAction;

	UPROPERTY( EditDefaultsOnly, Category = "LOCO|Input")
	TObjectPtr<UInputAction> JogAction;
	
//	UPROPERTY( EditDefaultsOnly, Category = "CB|Input")
//	TObjectPtr<UInputAction> DodgeAction;

	UPROPERTY( EditDefaultsOnly, Category = "LOCO|Movement")
	float WalkSpeed = 225.f; 

	UPROPERTY( EditDefaultsOnly, Category = "LOCO|Movement")
	float JogSpeed = 600.f;
	
	//Callback functions

	void Input_Move(const FInputActionValue& InputActionValue);
	void Input_Look(const FInputActionValue& InputActionValue);
	void Input_JumpStart();
	void Input_JumpEnd();
	void Input_Crouch();
	bool bJogHeld = false;
	void Input_JogStart();
	void Input_JogEnd();
	float GetWalkJogInputScale() const;
	//void Input_Dodge();
	

};
