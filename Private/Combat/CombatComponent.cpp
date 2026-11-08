// Fill out your copyright notice in the Description page of Project Settings.


#include "Combat/CombatComponent.h"

#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Net/UnrealNetwork.h"


UCombatComponent::UCombatComponent() //Constructor
{
	PrimaryComponentTick.bCanEverTick = true;

	bAiming = false;
}

void UCombatComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);
	APawn* OwningPawn = Cast<APawn>(GetOwner());
	if (!IsValid(OwningPawn) || !OwningPawn->IsLocallyControlled()) return;
	
	APlayerController* PC = Cast<APlayerController>(OwningPawn->GetController());
	if (!IsValid(PC)) return;
}

void UCombatComponent::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME_CONDITION(UCombatComponent, bAiming, COND_SkipOwner); //set locally so we can skip the owner
}

void UCombatComponent::Initiate_Aim_Pressed()
{
	Local_Aim(true); //sets local Aiming
	Server_Aim(true);//sets server Aiming
}

void UCombatComponent::Initiate_Aim_Released()
{
	Local_Aim(false); 
	Server_Aim(false);
}

void UCombatComponent::Server_Aim_Implementation(bool bPressed)
{
	Local_Aim(bPressed);
}

void UCombatComponent::Local_Aim(bool bPressed)
{
	bAiming = bPressed;	
	
	if (ACharacter* OwnerCharacter = Cast<ACharacter>(GetOwner())) //component lives on the character
	{
		if (UCharacterMovementComponent* CMC = OwnerCharacter->GetCharacterMovement()) // get its movement component
		{
			CMC->bOrientRotationToMovement = !bAiming; // not aiming = turn toward movement (free camera orbit at idle)
			CMC->bUseControllerDesiredRotation = false; // aiming = turn smoothly toward the camera, so A/D/S strafe
		}
		OwnerCharacter->bUseControllerRotationYaw = bAiming; // NEW: aiming = capsule matches camera yaw instantly (shooter behavior); runs on owner + server
	}
	OnAimingStatusChanged.Broadcast(bAiming);
}