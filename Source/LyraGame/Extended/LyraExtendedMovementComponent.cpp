// Fill out your copyright notice in the Description page of Project Settings.


#include "Extended/LyraExtendedMovementComponent.h"
#include "LyraExtendedCharacter.h"

void ULyraExtendedMovementComponent::PhysCustom(float deltaTime, int32 Iterations)
{
	Super::PhysCustom(deltaTime, Iterations);

	if (GetLyraExtendedCharacter()->IsClimbing())
	{
		const FClimbingMovementData& CurrentClimbingData = GetLyraExtendedCharacter()->GetClimbingMovementData();

		float ElapsedTime = GetWorld()->GetTimerManager().GetTimerElapsed(GetLyraExtendedCharacter()->GetClimbingTimer());
		float LerpAlpha = CurrentClimbingData.LerpAlphaCurve->GetFloatValue(ElapsedTime + CurrentClimbingData.TimeOffset);

		FVector NewLocation = FMath::Lerp(CurrentClimbingData.StartLocation, CurrentClimbingData.EndLocation, LerpAlpha);
		FRotator NewRotation = FMath::Lerp(CurrentClimbingData.StartRotation, CurrentClimbingData.EndRotation, LerpAlpha);

		FVector LocationDelta = NewLocation - GetActorLocation();
		Velocity = LocationDelta / deltaTime;
		FHitResult Hit;
		SafeMoveUpdatedComponent(LocationDelta, NewRotation, false, OUT Hit);
	}
}
