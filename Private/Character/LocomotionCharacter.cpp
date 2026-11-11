// Copyright Red Eye Digital, LLC. All Rights Reserved.

#include "Character/LocomotionCharacter.h"

#include "Data/Enums.h"
#include "EnhancedInputComponent.h"
#include "Engine/LocalPlayer.h"
#include "Camera/CameraComponent.h"
#include "Components/CapsuleComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/SpringArmComponent.h"
#include "GameFramework/Controller.h"
#include "Combat/CombatComponent.h"
#include "Kismet/KismetMathLibrary.h"
#include "Player/LocomotionPlayerController.h"


ALocomotionCharacter::ALocomotionCharacter()
{
	// Set size for collision capsule
	GetCapsuleComponent()->InitCapsuleSize(42.f, 96.0f);
		
	// Don't rotate when the controller rotates. Let that just affect the camera.
	bUseControllerRotationPitch = false;
	bUseControllerRotationYaw = false;
	bUseControllerRotationRoll = false;

	// Configure character movement
	GetCharacterMovement()->bOrientRotationToMovement = true;
	GetCharacterMovement()->RotationRate = FRotator(0.0f, 500.0f, 0.0f);

	GetCharacterMovement()->JumpZVelocity = 500.f;
	GetCharacterMovement()->AirControl = 0.35f;
	GetCharacterMovement()->MaxWalkSpeed = 600.f;
	GetCharacterMovement()->MinAnalogWalkSpeed = 20.f;
	GetCharacterMovement()->BrakingDecelerationWalking = 2000.f;
	GetCharacterMovement()->BrakingDecelerationFalling = 1500.0f;

	// Create a camera boom (pulls in towards the player if there is a collision)
	CameraBoom = CreateDefaultSubobject<USpringArmComponent>(TEXT("CameraBoom"));
	CameraBoom->SetupAttachment(RootComponent);
	CameraBoom->TargetArmLength = 300.0f;
	CameraBoom->bUsePawnControlRotation = true;

	// Create a follow camera
	FollowCamera = CreateDefaultSubobject<UCameraComponent>(TEXT("FollowCamera"));
	FollowCamera->SetupAttachment(CameraBoom, USpringArmComponent::SocketName);
	FollowCamera->bUsePawnControlRotation = false;
	
	// Combat
	Combat = CreateDefaultSubobject<UCombatComponent>("Combat");
	Combat->SetIsReplicated(true);
	
	TurningStatus = ETurningInPlace::NotTurning;
	
	// Replication
	bReplicates = true;

}

void ALocomotionCharacter::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

}

void ALocomotionCharacter::BeginPlay()
{
	Super::BeginPlay();
	
	if (ALocomotionPlayerController* PC = Cast<ALocomotionPlayerController>(GetController());IsValid(PC))
	{
		PC->bPawnAlive = true;
	}
}

void ALocomotionCharacter::Dodge(float Forward, float Right)
{
	if (GetCharacterMovement()->IsFalling() || IsDodging()) return; // the same gate
	UAnimMontage* Montage = GetDodgeMontage(Forward, Right); // choose the roll
	if (!Montage) return; // nothing chosen
	PlayAnimMontage(Montage); // play now on this machine so the owner sees no lag
	if (!HasAuthority()) // a client must also ask the server
	{
		Server_Dodge(Forward, Right); // the server plays the same montage
	}
	else
	{
		Multicast_Dodge(Montage); //the host tells every client to play the same montage
	}
}

void ALocomotionCharacter::Server_Dodge_Implementation(float Forward, float Right)
{
	if (GetCharacterMovement()->IsFalling() || IsDodging()) return; //the server re-checks and never trusts the client
	if (UAnimMontage* Montage = GetDodgeMontage(Forward, Right)) //choose again on the server
	{
		PlayAnimMontage(Montage); // plays on server
		Multicast_Dodge(Montage); //starts the same montage on every client so simulated proxies animate and move
	}
}

UAnimMontage* ALocomotionCharacter::GetDodgeMontage(float Forward, float Right) const
{
	constexpr float Deadzone = 0.1f; // ignore tiny values
	if (FMath::Abs(Forward) > Deadzone) // forward and back come first
	{
		return Forward > 0.f ? DodgeForwardMontage : DodgeBackMontage; // swap these two if your test shows them reversed
	}
	if (FMath::Abs(Right) > Deadzone) // otherwise use sideways
	{
		return Right > 0.f ? DodgeRightMontage : DodgeLeftMontage; 
	}
	return nullptr;
}

bool ALocomotionCharacter::IsDodging() const
{
	const UAnimInstance* AnimInstance = GetMesh() ? GetMesh()->GetAnimInstance() : nullptr; // get the anim instance safely
	if (!AnimInstance) return false; // nothing to check
	return AnimInstance->Montage_IsPlaying(DodgeForwardMontage)
	|| AnimInstance->Montage_IsPlaying(DodgeBackMontage)
	|| AnimInstance->Montage_IsPlaying(DodgeLeftMontage)
	|| AnimInstance->Montage_IsPlaying(DodgeRightMontage); // true if any of the four is playing
}

FRotator ALocomotionCharacter::GetFixedAimRotation() const
{
	FRotator AimRotation = GetBaseAimRotation();
	if (AimRotation.Pitch > 90.f && !IsLocallyControlled()) //this is for other players looking at our character
	{
		// map pitch from [270, 360] to [-90, 0]
		const FVector2D InRange(270.f, 360.f);
		const FVector2D OutRange(-90.f, 0.f);
		AimRotation.Pitch = FMath::GetMappedRangeValueClamped(InRange, OutRange, AimRotation.Pitch); //this will overwrite our AimRotation
	}
	return AimRotation; //Fixed Value
	
}

void ALocomotionCharacter::Multicast_Dodge_Implementation(UAnimMontage* Montage)
{
	if (HasAuthority() || IsLocallyControlled()) return; // the server and the owning player have already played it
	PlayAnimMontage(Montage); // other players' copies play it now  
}

void ALocomotionCharacter::Input_Aim_Pressed()
{
	if (!IsValid(Combat)) return;
	Combat->Initiate_Aim_Pressed();
	OnAim(true); //pressing right mouse button
	
}

void ALocomotionCharacter::Input_Aim_Released()
{
	if (!IsValid(Combat)) return;
	Combat->Initiate_Aim_Released();
	OnAim(false); //calling the OnAim Bool that will be implemented on blueprints
}

void ALocomotionCharacter::CalculateTurnInPlaceParameters(float DeltaTime)
{
	const bool bIsAiming = IsValid(Combat) && Combat->bAiming; // NEW: turn in place only applies while aiming (bAiming is replicated, so this works on all machines)
	if (!bIsAiming) // NEW: not aiming = free camera orbit, no turn in place
	{
		StartingAimRotation = FRotator(0.f, GetActorRotation().Yaw, 0.f); // NEW: track where the BODY faces, so pressing aim measures camera vs body
		AO_Yaw = 0.f; // NEW: no root offset while not aiming
		InterpAO_Yaw = 0.f; // NEW: clear any leftover interpolation
		TurningStatus = ETurningInPlace::NotTurning; // NEW: never report a turn while not aiming
		return;
	}
	//Get velocity, see if its zero
	FVector Velocity = GetVelocity();
	Velocity.Z = 0.f;
	float Speed = Velocity.Size(); //or use Size2d and you don't have to zero out the Z
	bool bIsInAir = GetCharacterMovement()->IsFalling();	// See if we are falling
	
	if (Speed == 0.f && !bIsInAir) 	// if standing still and not jumping
	{
		FRotator CurrentAimRotation(0.f, GetBaseAimRotation().Yaw, 0.f); //get current aim rotation
		FRotator DeltaAimRotation = UKismetMathLibrary::NormalizedDeltaRotator(CurrentAimRotation, StartingAimRotation); //get delta aim rotation, starting set in beginplay
		AO_Yaw = DeltaAimRotation.Yaw;  // Store the Yaw of the delta aim rotation (AO_Yaw0
		
		if (TurningStatus == ETurningInPlace::NotTurning)
		{
			InterpAO_Yaw = AO_Yaw; // set InterpAO_Yaw to AO_Yaw
		}
			
		TurnInPlace(DeltaTime); // interpolates the InterpAO_Yaw value to zero
	}
		
	if (Speed > 0.f || bIsInAir) //if running or jumping
	{
		StartingAimRotation = FRotator(0.f, GetBaseAimRotation().Yaw, 0.f); //reset initial aim rotation to the current actual aim rotation
		AO_Yaw = 0.f;
		
		FRotator AimRotation = GetBaseAimRotation();
		FRotator MovementRotation = UKismetMathLibrary::MakeRotFromX(GetVelocity());
		MovementOffsetYaw = UKismetMathLibrary::NormalizedDeltaRotator(MovementRotation, AimRotation).Yaw; //delta between our movement rotation and our aim rotation
		TurningStatus = ETurningInPlace::NotTurning;
	}
	AO_Yaw *= -1.f; //makes sure AO_Yaw is not backwards
}

void ALocomotionCharacter::TurnInPlace(float DeltaTime)
{
	if (AO_Yaw > 90.f) //if AO_Yaw >90
	{
		TurningStatus = ETurningInPlace::Right;
	}
	else if (AO_Yaw < -90.f)
	{
		TurningStatus = ETurningInPlace::Left;
	}
	if (TurningStatus !=ETurningInPlace::NotTurning) //we are turning 
	{
		InterpAO_Yaw = FMath::FInterpTo( InterpAO_Yaw, 0.f, DeltaTime, 4.0F);
		AO_Yaw = InterpAO_Yaw;
		if (FMath::Abs(AO_Yaw) < 5.f)
		{
			TurningStatus = ETurningInPlace::NotTurning;
			StartingAimRotation = FRotator(0.f, GetBaseAimRotation().Yaw, 0.f); //reset initial aim rotation to our actual aim rotation
		}
	}
}

void ALocomotionCharacter::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
	CalculateTurnInPlaceParameters(DeltaTime);
}

void ALocomotionCharacter::PossessedBy(AController* EventController)
{
	Super::PossessedBy(EventController);
}

void ALocomotionCharacter::SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);
	
	UEnhancedInputComponent* LocoInputComponent = Cast<UEnhancedInputComponent>(PlayerInputComponent);

	LocoInputComponent->BindAction(AimWeaponAction, ETriggerEvent::Started, this, &ThisClass::Input_Aim_Pressed);
	LocoInputComponent->BindAction(AimWeaponAction, ETriggerEvent::Completed, this, &ThisClass::Input_Aim_Released);
	
}

void ALocomotionCharacter::OnRep_PlayerState()
{
	Super::OnRep_PlayerState();
	if (ALocomotionPlayerController* PC = Cast<ALocomotionPlayerController>(GetController()); IsValid(PC))
	{
		PC->bPawnAlive = true;
	}
}