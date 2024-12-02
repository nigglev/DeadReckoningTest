// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/WorldSettings.h"
#include "DRWorldSettings.generated.h"

/**
 * 
 */
UCLASS()
class DEADRECKONINGTEST_API ADRWorldSettings : public AWorldSettings
{
	GENERATED_BODY()

public:

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Motion Replication")
	float ReplicationTime = 0.5f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Motion Type")
	bool IsCircleMovement = true;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Circular Motion")
	float Radius = 300.0f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Circular Motion")
	FRotator AngularSpeed = FRotator(0.0f, 90.0f, 0.0f);
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Square Motion")
	float SideLength = 300.0f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Square Motion")
	float Speed = 200.0f;
};
