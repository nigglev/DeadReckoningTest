// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Components/TextBlock.h"
#include "InfoWidget.generated.h"

/**
 * 
 */
UCLASS()
class DEADRECKONINGTEST_API UInfoWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	void UpdateAverageServerUpdateTimeText(float InAverageServerUpdateTime) const;
	void UpdateMotionInfoText(bool IsCircle, float InCircleRadius, float InAngularSpeed, float InSquareSideLength, float Speed) const;

protected:
	UPROPERTY(meta = (BindWidget))
	class UTextBlock* AverageServerUpdateTimeText;

	UPROPERTY(meta = (BindWidget))
	class UTextBlock* MotionInfoText;
	
};
