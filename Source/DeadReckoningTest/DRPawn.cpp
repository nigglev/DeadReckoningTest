// Fill out your copyright notice in the Description page of Project Settings.


#include "DRPawn.h"

#include "Camera/CameraComponent.h"
#include "GameFramework/PlayerStart.h"
#include "GameFramework/SpringArmComponent.h"
#include "Kismet/GameplayStatics.h"
#include "Net/UnrealNetwork.h"


FKinematicState::FKinematicState()
{
	Position = FVector::ZeroVector;
	Velocity = FVector::ZeroVector;
	Acceleration = FVector::ZeroVector;
}

FKinematicState::FKinematicState(const FVector& In_Position, const FVector& In_Velocity, const FVector& In_Acceleration)
{
	Position = In_Position;
	Velocity = In_Velocity;
	Acceleration = In_Acceleration;
}

bool FKinematicState::NetSerialize(FArchive& Ar, class UPackageMap* Map, bool& bOutSuccess)
{
	Ar << Position;
	Ar << Velocity;
	Ar << Acceleration;
	bOutSuccess = true;
	return true;
}

FString FKinematicState::ToString() const
{
	TStringBuilder<256> SB;
	SB.Appendf(TEXT("Position: %s "), *Position.ToCompactString());
	SB.Appendf(TEXT("Velocity: %s [%f] "), *Velocity.ToCompactString(), Velocity.Length());
	SB.Appendf(TEXT("Acceleration: %f"), Acceleration.Length() * FMath::Sign(Acceleration.X));
	return SB.ToString();
}

ADRPawn::ADRPawn() : TimeStampCollector(1.0f, 1.0f)
{
	
	PrimaryActorTick.bCanEverTick = true;
	
	bReplicates = true;
	NetUpdateFrequency = 100;
	
	BaseMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("DRMesh"));
	RootComponent = BaseMesh;
	BaseMesh->SetupAttachment(RootCapsuleComponent);

	CameraBoom = CreateDefaultSubobject<USpringArmComponent>(TEXT("CameraBoom"));
	CameraBoom->bUsePawnControlRotation = false;

	FollowCamera = CreateDefaultSubobject<UCameraComponent>(TEXT("FollowCamera"));
	FollowCamera->SetupAttachment(CameraBoom, USpringArmComponent::SocketName);
	FollowCamera->bUsePawnControlRotation = false;

	// Initialize variables for dead reckoning
	ServerUpdateTime = 1 / NetUpdateFrequency;
	AverageServerUpdateTime = 1 / NetUpdateFrequency;
	DeadReckon_T = 0.f;
	Server_T_SinceLastFrame = 0.f;
}

void ADRPawn::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME_CONDITION_NOTIFY(ADRPawn, Server_KinematicState, COND_None, REPNOTIFY_OnChanged);
}


void ADRPawn::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);
}


void ADRPawn::BeginPlay()
{
	Super::BeginPlay();
	FVector PlayerStartPosition = GetPlayerStartPosition();

	// Initialize motion parameters
	Radius = GetDRWorldSettings()->Radius;
	AngularSpeed = GetDRWorldSettings()->AngularSpeed;
	
	SideLength = GetDRWorldSettings()->SideLength;
	Speed = GetDRWorldSettings()->Speed;

	ReplicationTime = GetDRWorldSettings()->ReplicationTime;
	ReplicationDistSquare = Speed * ReplicationTime;
	ReplicationDistCircle = ReplicationTime * FMath::DegreesToRadians(AngularSpeed.Yaw) * Radius;

	// Setup camera properties
	CameraBoom->TargetArmLength = CameraDistance;
	CameraBoom->SetRelativeLocation(FVector(0, 0, CameraSpringZLocation));

	CenterCircleMovement = PlayerStartPosition;
	StartPositionSquareMovement = FVector(PlayerStartPosition.X - SideLength / 2, PlayerStartPosition.Y - SideLength / 2, 0);
	CurrentVelocity = FVector::ForwardVector * Speed;
	
	if(!GetDRWorldSettings()->IsCircleMovement)
	{
		DrawDebugLifetime = 4 * SideLength / Speed;
		SetActorLocation(StartPositionSquareMovement);
	}
	else
	{
		DrawDebugLifetime = 360.0f / AngularSpeed.Yaw;
	}
	
	if(HasAuthority())	
	{	
		Server_KinematicState = FKinematicState(GetActorLocation(), FVector::Zero(), FVector::Zero());
	}
	else
	{
		GetDRController()->UpdateMotionInfoWidget(GetDRWorldSettings()->IsCircleMovement, Radius, AngularSpeed.Yaw, SideLength, Speed);
		Client_KinematicState = FKinematicState(GetActorLocation(), FVector::Zero(), FVector::Zero());
	}
	
}

// Get a reference to the custom controller
ADRController* ADRPawn::GetDRController() const
{
	if(Controller == nullptr)
		return nullptr;
	return Cast<ADRController>(Controller);
}

// Retrieve world settings
ADRWorldSettings* ADRPawn::GetDRWorldSettings() const
{
	ADRWorldSettings* WorldSettings = Cast<ADRWorldSettings>(GetWorldSettings());
	return WorldSettings;
}

// Find the starting position for the player
FVector ADRPawn::GetPlayerStartPosition() const
{
	TArray<AActor*> PlayerStarts;
	UGameplayStatics::GetAllActorsOfClass(GetWorld(), APlayerStart::StaticClass(), PlayerStarts);
	
	if (PlayerStarts.Num() > 0)
	{
		APlayerStart* PlayerStart = Cast<APlayerStart>(PlayerStarts[0]);
		if (PlayerStart)
		{
			FVector Position = PlayerStart->GetActorLocation();
			return Position;
		}
	}

	UE_LOG(LogTemp, Warning, TEXT("No PlayerStart found in the world!"));
	return FVector::ZeroVector;
}

// Tick function to handle movement logic
void ADRPawn::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
	
	if(HasAuthority())
	{
		if(GetDRWorldSettings()->IsCircleMovement)
			MoveСircleServer(DeltaTime);
		else
			MoveSquareServer(DeltaTime);
	}
	else
	{
		DeadReckon_T += DeltaTime;
		DeadReckon_T_Hat = DeadReckon_T / AverageServerUpdateTime;
		if(DeadReckon_T_Hat > MaxDeadReckon_T_Hat)
		{
			DeadReckon_T_Hat = MaxDeadReckon_T_Hat;
		}
		DeadReckoningMove(DeltaTime);
	}

}

// Logic for moving in a circular path on the server
void ADRPawn::MoveСircleServer(float In_DeltaTime)
{
	const FRotator Rot = AngularSpeed * In_DeltaTime;
	CurrentDirection = Rot.RotateVector(CurrentDirection);
	CurrentDirection.Normalize();

	const FVector PreviousLocation = GetActorLocation();
	const FVector NewLocation = CenterCircleMovement + CurrentDirection * Radius;

	const float v = FMath::DegreesToRadians(AngularSpeed.Yaw) * Radius;
	const FVector Tangent = FVector::UpVector.Cross(CurrentDirection);
	const FVector Velocity = Tangent * v;

	FVector VectorA = NewLocation - CenterCircleMovement;
	FVector VectorB = Server_KinematicState.Position - CenterCircleMovement;

	VectorA.Normalize();
	VectorB.Normalize();

	float AngleInRadians = FMath::Acos(FVector::DotProduct(VectorA, VectorB));
	float ArcLength = Radius * AngleInRadians;
	
	 if(ArcLength > ReplicationDistCircle)
	 {
	 	Server_KinematicState.Velocity = Velocity;
	 	Server_KinematicState.Position = NewLocation;
	 }
	SetActorLocation(NewLocation);
	CustomDrawDebugLine(PreviousLocation, NewLocation, FColor::Green, 5.0f, 10.0f);
}

// Logic for square path movement
void ADRPawn::MoveSquareServer(float In_DeltaTime)
{
	const FVector PreviousLocation = GetActorLocation();
	FVector Position = PreviousLocation + CurrentVelocity * In_DeltaTime;
	
	float DistStep = CurrentVelocity.Size() * In_DeltaTime;
	PassedDistance += DistStep;
	
	float DeltaDist = SideLength - PassedDistance;
	if(DeltaDist < 0)
	{
		Position += DeltaDist * CurrentVelocity.GetSafeNormal();
		PassedDistance = 0.0f;
		CurrentVelocity = TurnRight.RotateVector(CurrentVelocity);
	}
	SetActorLocation(Position);
	
	float Dist = FVector::Distance(Position, Server_KinematicState.Position);
	if(Dist > ReplicationDistSquare)
	{	
		Server_KinematicState.Velocity = CurrentVelocity;
		Server_KinematicState.Position = Position;
	}
	
	CustomDrawDebugLine(PreviousLocation, GetActorLocation(), FColor::Green, 5.0f, 10.0f);
}

// Logic for dead reckoning movement on the client
void ADRPawn::DeadReckoningMove(float In_DeltaTime)
{
	Client_KinematicState.Acceleration = Server_KinematicState.Acceleration;
	const FVector P1 = Client_KinematicState.Position + Client_KinematicState.Velocity * In_DeltaTime + Client_KinematicState.Acceleration * In_DeltaTime * In_DeltaTime * 0.5;
	const FVector P2 = Server_KinematicState.Position + Server_KinematicState.Velocity * In_DeltaTime + Server_KinematicState.Acceleration * In_DeltaTime * In_DeltaTime * 0.5;
	FVector DeadReckonedPos = P1;
	FVector DeltaP = P2 - P1;
	DeadReckonedPos += DeltaP * DeadReckon_T_Hat;

	FVector OldPos = Client_KinematicState.Position;
	Client_KinematicState.Position = DeadReckonedPos;
	SetActorLocation(Client_KinematicState.Position);
	
	Server_KinematicState.Position += Server_KinematicState.Velocity * In_DeltaTime + Server_KinematicState.Acceleration * In_DeltaTime * In_DeltaTime / 2;
	Server_KinematicState.Velocity = Server_KinematicState.Velocity + Server_KinematicState.Acceleration * In_DeltaTime;
	
	Client_KinematicState.Velocity = Client_KinematicState.Velocity + Client_KinematicState.Acceleration * In_DeltaTime;
	Client_KinematicState.Velocity = Client_KinematicState.Velocity + (Server_KinematicState.Velocity - Client_KinematicState.Velocity) * DeadReckon_T_Hat;

	DrawShape(OldPos, DeadReckonedPos, FColor::Red, 5.0f);
	
}

// Callback when the server kinematic state is replicated
void ADRPawn::OnRep_KinematicState()
{
	// Reset dead reckoning timer and update server timing
	DeadReckon_T = 0;
	TimeStampCollector.Add(FDateTime::UtcNow());
	if(TimeStampCollector.IsValid())
		AverageServerUpdateTime = TimeStampCollector.GetAverageDuration();
	
	GetDRController()->UpdateAverageServerUpdateTimeInfoWidget(AverageServerUpdateTime);
	
	float PointRadius = FMath::Min(10.f, 0.3f * ReplicationDistSquare);
	DrawDebugSphere(GetWorld(),	GetActorLocation(), PointRadius, 12, FColor::Red, false, DrawDebugLifetime, 0, 2.0f);
	DrawDebugSphere(GetWorld(),	Server_KinematicState.Position, PointRadius, 12, FColor::Yellow, false, DrawDebugLifetime, 0, 2.0f);
}

void ADRPawn::CustomDrawDebugLine(const FVector& From, const FVector& To, FColor Color, float Thickness, float InLifeTime) const
{
	DrawDebugLine(
		GetWorld(),
		From,
		To,
		Color,
		false,
		InLifeTime,      
		0,           
		Thickness       
	);
}

void ADRPawn::DrawShape(const FVector& OldPos, const FVector& NewPos, FColor Color, float Thickness) const
{
	if(GetDRWorldSettings()->IsCircleMovement)
		CustomDrawDebugLine(OldPos, NewPos, Color, Thickness, DrawDebugLifetime);
	else
		CustomDrawDebugLine(OldPos, NewPos, Color, Thickness, DrawDebugLifetime);
}




