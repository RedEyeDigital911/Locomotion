// Copyright Epic Games, Inc. All Rights Reserved.


#include "Player/LocomotionPlayerController.h"

#include "EnhancedInputComponent.h"
#include "EnhancedInputSubsystems.h"
#include "Engine/LocalPlayer.h"
#include "InputMappingContext.h"
#include "Character/LocomotionCharacter.h"
#include "GameFramework/CharacterMovementComponent.h"

ALocomotionPlayerController::ALocomotionPlayerController()
{
	bReplicates = true;
	bPawnAlive = true;
}

void ALocomotionPlayerController::OnRep_PlayerState()
{
	Super::OnRep_PlayerState();
	OnPlayerStateReplicated.Broadcast();
}

void ALocomotionPlayerController::BeginPlay()
{
	Super::BeginPlay();
	UEnhancedInputLocalPlayerSubsystem* Subsystem = ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(GetLocalPlayer());
	if (IsValid(Subsystem))
	{
		Subsystem->AddMappingContext(IMC_Player, 0);
	}
}

void ALocomotionPlayerController::SetupInputComponent()
{
	Super::SetupInputComponent();
	UEnhancedInputComponent* EnhancedInputComponent = CastChecked<UEnhancedInputComponent>(InputComponent);
	
	EnhancedInputComponent->BindAction(MoveAction, ETriggerEvent::Triggered, this, &ThisClass::Input_Move);
	EnhancedInputComponent->BindAction(LookAction, ETriggerEvent::Triggered, this, &ThisClass::Input_Look);
	EnhancedInputComponent->BindAction(JumpAction, ETriggerEvent::Started, this, &ThisClass::Input_JumpStart);
	EnhancedInputComponent->BindAction(JumpAction, ETriggerEvent::Completed, this, &ThisClass::Input_JumpEnd);
	EnhancedInputComponent->BindAction(CrouchAction, ETriggerEvent::Started, this, &ThisClass::Input_Crouch);
	EnhancedInputComponent->BindAction(JogAction, ETriggerEvent::Started, this, &ThisClass::Input_JogStart);
	EnhancedInputComponent->BindAction(JogAction, ETriggerEvent::Completed, this, &ThisClass::Input_JogEnd);
	EnhancedInputComponent->BindAction(DodgeAction, ETriggerEvent::Completed, this, &ThisClass::Input_Dodge); 

}

void ALocomotionPlayerController::OnPossess(APawn* InPawn)
{
	Super::OnPossess(InPawn);
	bPawnAlive = true;
}

void ALocomotionPlayerController::Input_Move(const FInputActionValue& InputActionValue)
{
	if (!bPawnAlive) return;

	const FVector2D InputAxisVector = InputActionValue.Get<FVector2D>().GetClampedToMaxSize(1.0);
	const float SpeedScale = GetWalkJogInputScale();

	const FRotator Rotation = GetControlRotation();
	const FRotator YawRotation(0.f, Rotation.Yaw, 0.f);
	const FVector ForwardDirection = FRotationMatrix(YawRotation).GetUnitAxis(EAxis::X);
	const FVector RightDirection = FRotationMatrix(YawRotation).GetUnitAxis(EAxis::Y);

	if (APawn* ControlledPawn = GetPawn())
	{
		ControlledPawn->AddMovementInput(ForwardDirection, InputAxisVector.Y * SpeedScale);
		ControlledPawn->AddMovementInput(RightDirection, InputAxisVector.X * SpeedScale);
	} 
}

void ALocomotionPlayerController::Input_Look(const FInputActionValue& InputActionValue)
{
	if (!bPawnAlive) return;

	const FVector2D InputAxisVector = InputActionValue.Get<FVector2D>();
	AddYawInput(InputAxisVector.X);
	AddPitchInput(InputAxisVector.Y);
}

void ALocomotionPlayerController::Input_JumpStart()
{
	if (!IsValid(GetCharacter())) return;
	if (!bPawnAlive) return;
	UCharacterMovementComponent* CMC = GetCharacter()->GetCharacterMovement();
	if (!IsValid(CMC)) return;

	if (CMC->bWantsToCrouch)
	{
		CMC->bWantsToCrouch = false;
	}
	else
	{
		GetCharacter()->Jump();
	}
}

void ALocomotionPlayerController::Input_JumpEnd()
{
	GetCharacter()->StopJumping();
}

void ALocomotionPlayerController::Input_Crouch()
{
	if (!IsValid(GetCharacter())) return;
	if (!bPawnAlive) return;

	if (UCharacterMovementComponent* CMC = GetCharacter()->GetCharacterMovement(); IsValid(CMC))
	{
		CMC->bWantsToCrouch = !CMC->bWantsToCrouch;
	}
}

void ALocomotionPlayerController::Input_JogStart()
{
	bJogHeld = true;
}

void ALocomotionPlayerController::Input_JogEnd()
{
	bJogHeld = false;
}

float ALocomotionPlayerController::GetWalkJogInputScale() const
{
	const ACharacter* ControlledCharacter = GetCharacter(); 
	if (!IsValid(ControlledCharacter)) return 1.f;    
	const UCharacterMovementComponent* CMC = ControlledCharacter->GetCharacterMovement(); 
	if (!IsValid(CMC) || CMC->MaxWalkSpeed <= 0.f) return 1.f;  // guards the division below

	if (ControlledCharacter->bIsCrouched) return 1.f;   // the crouch guard, so Blueprint crouch speed isn't scaled
	
	const float TargetSpeed = bJogHeld ? JogSpeed : WalkSpeed; //  pick the target speed from two independent numbers.

	return FMath::Clamp(TargetSpeed / CMC->MaxWalkSpeed, 0.f, 1.f); 	// the target (walk or jog) as a fraction of the ceiling, kept between 0 and 1.
}

void ALocomotionPlayerController::Input_Dodge()
{
	if (!bPawnAlive) return; // no dodging when dead
	ALocomotionCharacter* LOCOCharacter = Cast<ALocomotionCharacter>(GetCharacter()); // we need our own character class
	if (!IsValid(LOCOCharacter)) return; // safety check
	const FVector LocalInput = LOCOCharacter->GetActorRotation().UnrotateVector(LOCOCharacter->GetCharacterMovement()->GetCurrentAcceleration()).GetSafeNormal(); //reads the movement component's acceleration instead of the last input vector, converted to character space (X forward, Y right) and normalized); 
	LOCOCharacter->Dodge(LocalInput.X, LocalInput.Y); // hand the two values to the character
}


