// Fill out your copyright notice in the Description page of Project Settings.


#include "Extended/LyraExtendedMovementComponent.h"
#include "LyraExtendedCharacter.h"
#include "Curves/CurveVector.h"

void ULyraExtendedMovementComponent::PhysCustom(float deltaTime, int32 Iterations)
{
	Super::PhysCustom(deltaTime, Iterations);

	if (GetLyraExtendedCharacter()->IsClimbing())
	{
		const FClimbingMovementData& CurrentClimbingData = GetLyraExtendedCharacter()->GetClimbingMovementData();

		float ElapsedTime = GetWorld()->GetTimerManager().GetTimerElapsed(GetLyraExtendedCharacter()->GetClimbingTimer());
		FVector CurveValue = CurrentClimbingData.Curve->GetVectorValue(ElapsedTime + CurrentClimbingData.TimeOffset);
		float LerpAlpha = CurveValue.X;
		float NormalCorrectionAlpha = CurveValue.Y;
		float ZCorrectionAlpha = CurveValue.Z;

		FVector CorrectedStartLocation = FMath::Lerp(CurrentClimbingData.StartLocation, CurrentClimbingData.StartAnimationLocation, NormalCorrectionAlpha);
		CorrectedStartLocation.Z = FMath::Lerp(CurrentClimbingData.StartLocation.Z, CurrentClimbingData.StartAnimationLocation.Z, ZCorrectionAlpha);

		FVector NewLocation = FMath::Lerp(CorrectedStartLocation, CurrentClimbingData.EndLocation, LerpAlpha);
		FRotator NewRotation = FMath::Lerp(CurrentClimbingData.StartRotation, CurrentClimbingData.EndRotation, LerpAlpha);

		FVector LocationDelta = NewLocation - GetActorLocation();
		Velocity = LocationDelta / deltaTime;
		FHitResult Hit;
		SafeMoveUpdatedComponent(LocationDelta, NewRotation, false, OUT Hit);
	}
}
