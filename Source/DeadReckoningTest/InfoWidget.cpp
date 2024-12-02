// Fill out your copyright notice in the Description page of Project Settings.


#include "InfoWidget.h"


void UInfoWidget::UpdateAverageServerUpdateTimeText(float InAverageServerUpdateTime) const
{
	if (AverageServerUpdateTimeText)
	{
		FString LocationString = FString::Printf(TEXT("Average Server Update Time: %f"),
												 InAverageServerUpdateTime);
		AverageServerUpdateTimeText->SetText(FText::FromString(LocationString));
	}
}

void UInfoWidget::UpdateMotionInfoText(bool IsCircle, float InCircleRadius, float InAngularSpeed, float InSquareSideLength, float Speed) const
{
	if (MotionInfoText)
	{
		FString MotionInfoString;
		if(IsCircle)
			MotionInfoString = FString::Printf(TEXT("Circle Motion\nRadius = %.1f, Speed %.1f degrees per second"),
												 InCircleRadius, InAngularSpeed);
		else
		{
			MotionInfoString = FString::Printf(TEXT("Square Motion\nSide Length = %.1f, Speed %.1f"),
												 InSquareSideLength, Speed);
		}
		MotionInfoText->SetText(FText::FromString(MotionInfoString));
	}
}


