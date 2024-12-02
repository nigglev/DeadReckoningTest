// Fill out your copyright notice in the Description page of Project Settings.


#include "DRController.h"

#include "Blueprint/UserWidget.h"


void ADRController::BeginPlay()
{
	Super::BeginPlay();
	
	if (InfoWidgetClass && IsLocalController())
	{
		InfoWidget = CreateWidget<UInfoWidget>(this, InfoWidgetClass);
		if (InfoWidget != nullptr)
		{
			InfoWidget->AddToViewport();
		}
	}
}

void ADRController::UpdateAverageServerUpdateTimeInfoWidget(float InAverageServerUpdateTime) const
{
	if (InfoWidget != nullptr)
	{
		InfoWidget->UpdateAverageServerUpdateTimeText(InAverageServerUpdateTime);
	}
}

void ADRController::UpdateMotionInfoWidget(bool IsCircle, float InCircleRadius, float InAngularSpeed, float InSquareSideLength, float Speed) const
{
	if (InfoWidget != nullptr)
	{
		InfoWidget->UpdateMotionInfoText(IsCircle, InCircleRadius, InAngularSpeed, InSquareSideLength, Speed);
	}
}
