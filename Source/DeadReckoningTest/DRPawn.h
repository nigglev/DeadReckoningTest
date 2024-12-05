// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "DRController.h"
#include "DRWorldSettings.h"
#include "Components/CapsuleComponent.h"
#include "GameFramework/Pawn.h"
#include "Utilities/TimeDataCollector.h"
#include "DRPawn.generated.h"

USTRUCT()
struct FKinematicState
{
	GENERATED_BODY()
	
	UPROPERTY()
	FVector Position;
	UPROPERTY()
	FVector Velocity;
	UPROPERTY()
	FVector Acceleration;

	FKinematicState();
	FKinematicState(const FVector& In_Position, const FVector& In_Velocity, const FVector& In_Acceleration);
	bool NetSerialize(FArchive& Ar, class UPackageMap* Map, bool& bOutSuccess);
	FString ToString() const;
	
};

template<>
struct TStructOpsTypeTraits<FKinematicState> : public TStructOpsTypeTraitsBase2<FKinematicState>
{
	enum
	{
		WithNetSerializer = true
	};
};

UCLASS()
class DEADRECKONINGTEST_API ADRPawn : public APawn
{
	GENERATED_BODY()

public:
	
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UCapsuleComponent> RootCapsuleComponent;
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, meta = (AllowPrivateAccess = "true"))
	UStaticMeshComponent* BaseMesh;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = Camera, meta = (AllowPrivateAccess = "true"))
	class USpringArmComponent* CameraBoom;
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = Camera, meta = (AllowPrivateAccess = "true"))
	class UCameraComponent* FollowCamera;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Camera, meta = (AllowPrivateAccess = "true"))
	float CameraDistance = 0.0f;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Camera, meta = (AllowPrivateAccess = "true"))
	float CameraSpringZLocation = 1500.0f;
	
	ADRPawn();
	virtual void Tick(float DeltaTime) override;
	virtual void SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent) override;

protected:

	float Radius = 0.0f;
	FRotator AngularSpeed = FRotator::ZeroRotator;
	FVector CenterCircleMovement;
	FVector CurrentDirection = FVector::ForwardVector;

	float SideLength = 0.0f;
	float Speed = 0.0f;
	FVector CurrentVelocity;
	float PassedDistance = 0.0f;
	FRotator TurnRight = FRotator(0,90.0f,0);
	FVector StartPositionSquareMovement;
	int32 CurrentSide = 0;
	FVector TargetLocation;
	float DrawDebugLifetime = 0.0f;

	float ReplicationTime;
	float ReplicationDistCircle = 50.0f;
	float ReplicationDistSquare = 50.0f;
		
	UPROPERTY(ReplicatedUsing = OnRep_KinematicState)
	FKinematicState Server_KinematicState;
	UPROPERTY()
	FKinematicState Client_KinematicState;

	virtual void BeginPlay() override;
	ADRController* GetDRController() const;
	ADRWorldSettings* GetDRWorldSettings() const;
	FVector GetPlayerStartPosition() const;
	void MoveСircleServer(float In_DeltaTime);
	void MoveSquareServer(float In_DeltaTime);
	void DeadReckoningMove(float In_DeltaTime);
	void CustomDrawDebugLine(const FVector& From, const FVector& To, FColor Color, float Thickness, float InLifeTime) const;
	void DrawShape(const FVector& OldPos, const FVector& NewPos, FColor Color, float Thickness) const;
	
	UFUNCTION()
	void OnRep_KinematicState();

private:	
	
	FDateTimeStampCollector TimeStampCollector;
	float Server_T_SinceLastFrame;
	float DeadReckon_T;
	float DeadReckon_T_Hat;
	float ServerUpdateTime;
	float AverageServerUpdateTime;
	
};
