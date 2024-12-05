#pragma once

#include "CoreMinimal.h"
#include "DRController.h"
#include "DRWorldSettings.h"
#include "Components/CapsuleComponent.h"
#include "GameFramework/Pawn.h"
#include "Utilities/TimeDataCollector.h"
#include "DRPawn.generated.h"

// Structure representing the state of motion (position, velocity, acceleration)
USTRUCT()
struct FKinematicState
{
	GENERATED_BODY()
	
	UPROPERTY()
	FVector Position; // Current position of the pawn
	UPROPERTY()
	FVector Velocity; // Current velocity of the pawn
	UPROPERTY()
	FVector Acceleration; // Current acceleration of the pawn

	FKinematicState(); // Default constructor
	FKinematicState(const FVector& In_Position, const FVector& In_Velocity, const FVector& In_Acceleration); // Parameterized constructor

	// Serialize the kinematic state for network transmission
	bool NetSerialize(FArchive& Ar, class UPackageMap* Map, bool& bOutSuccess);

	// Debug string representation of the state
	FString ToString() const;
};

template<>
struct TStructOpsTypeTraits<FKinematicState> : public TStructOpsTypeTraitsBase2<FKinematicState>
{
	enum
	{
		WithNetSerializer = true // Enable network serialization
	};
};

UCLASS()
class DEADRECKONINGTEST_API ADRPawn : public APawn
{
	GENERATED_BODY()

public:

	// Components for the pawn's structure and visuals
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UCapsuleComponent> RootCapsuleComponent;
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, meta = (AllowPrivateAccess = "true"))
	UStaticMeshComponent* BaseMesh;

	// Camera components for controlling player view
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = Camera, meta = (AllowPrivateAccess = "true"))
	class USpringArmComponent* CameraBoom;
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = Camera, meta = (AllowPrivateAccess = "true"))
	class UCameraComponent* FollowCamera;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Camera, meta = (AllowPrivateAccess = "true"))
	float CameraDistance = 0.0f; // Distance between the camera and the pawn
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Camera, meta = (AllowPrivateAccess = "true"))
	float CameraSpringZLocation = 1500.0f; // Z offset for the spring arm

	ADRPawn();
	virtual void Tick(float DeltaTime) override; // Called every frame
	virtual void SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent) override; // Setup for input bindings

protected:

	// Circle movement properties
	float Radius = 0.0f; // Radius of circular motion
	FRotator AngularSpeed = FRotator::ZeroRotator; // Angular speed in degrees per second
	FVector CenterCircleMovement; // Center of the circular path
	FVector CurrentDirection = FVector::ForwardVector; // Current direction vector for movement

	// Square movement properties
	float SideLength = 0.0f; // Length of each side of the square path
	float Speed = 0.0f; // Linear speed of the pawn
	FVector CurrentVelocity; // Current velocity vector
	float PassedDistance = 0.0f; // Distance passed along the current side
	FRotator TurnRight = FRotator(0,90.0f,0); // Rotation for turning at corners
	FVector StartPositionSquareMovement; // Starting position for square movement
	int32 CurrentSide = 0; // Current side of the square being traversed
	float DrawDebugLifetime = 0.0f; // Lifetime for debug visuals

	// Dead reckoning properties
	UPROPERTY(ReplicatedUsing = OnRep_KinematicState)
	FKinematicState Server_KinematicState; // Server's authoritative state
	UPROPERTY()
	FKinematicState Client_KinematicState; // Client's predicted state

	// Replication settings
	float ReplicationTime;
	float ReplicationDistCircle = 50.0f; // Distance threshold for replicating in circular motion
	float ReplicationDistSquare = 50.0f; // Distance threshold for replicating in square motion

	// Function overrides and helpers
	virtual void BeginPlay() override;
	ADRController* GetDRController() const; // Get the custom controller
	ADRWorldSettings* GetDRWorldSettings() const; // Get world-specific settings
	FVector GetPlayerStartPosition() const; // Retrieve the initial spawn position

	// Movement implementations
	void MoveСircleServer(float In_DeltaTime); // Server-side logic for circular motion
	void MoveSquareServer(float In_DeltaTime); // Server-side logic for square motion
	void DeadReckoningMove(float In_DeltaTime); // Client-side dead reckoning logic

	// Debug drawing utilities
	void CustomDrawDebugLine(const FVector& From, const FVector& To, FColor Color, float Thickness, float InLifeTime) const;
	void DrawShape(const FVector& OldPos, const FVector& NewPos, FColor Color, float Thickness) const;

	UFUNCTION()
	void OnRep_KinematicState(); // Callback for when Server_KinematicState replicates

private:

	// Time synchronization utilities
	FDateTimeStampCollector TimeStampCollector;
	float Server_T_SinceLastFrame;
	float DeadReckon_T;
	float DeadReckon_T_Hat;
	const float MaxDeadReckon_T_Hat = 1.2f;
	float ServerUpdateTime;
	float AverageServerUpdateTime;
};
