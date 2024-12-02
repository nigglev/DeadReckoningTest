// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "InfoWidget.h"
#include "GameFramework/PlayerController.h"
#include "DRController.generated.h"

/**
 * 
 */
UCLASS()
class DEADRECKONINGTEST_API ADRController : public APlayerController
{
	GENERATED_BODY()

public:
	void UpdateAverageServerUpdateTimeInfoWidget(float InAverageServerUpdateTime) const;
	void UpdateMotionInfoWidget(bool IsCircle, float InCircleRadius, float InAngularSpeed, float InSquareSideLength, float Speed) const;
	
protected:
	virtual void BeginPlay() override;
	
private:
	UPROPERTY(EditAnywhere, Category = "HUD")
	TSubclassOf<UInfoWidget> InfoWidgetClass;

	TWeakObjectPtr<UInfoWidget> InfoWidget;
};
