// Fill out your copyright notice in the Description page of Project Settings.


#include "Extended/LyraExtendedCharacter.h"
#include "Components/CapsuleComponent.h"
#include "LyraExtendedMovementComponent.h"

#define ECC_Climbing ECC_GameTraceChannel6

namespace DebugCVars
{
	static bool ShouldDrawClimbingDebug = false;
	static FAutoConsoleVariableRef CVarShouldDrawClimbingDebug(TEXT("Climbing.DrawDebugInfo"),
		ShouldDrawClimbingDebug,
		TEXT("Should we draw climbing debug info?"));

	static float ClimbingDebugLifetime = 2.0f;
	static FAutoConsoleVariableRef CVarClimbingDebugLifetime(TEXT("Climbing.DebugInfoLifetime"),
		ClimbingDebugLifetime,
		TEXT("Climbing debug info lifetime"));
}

static FName NAME_LyraCharacterCollisionProfile_Capsule(TEXT("LyraPawnCapsule"));

ALyraExtendedCharacter::ALyraExtendedCharacter(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer.SetDefaultSubobjectClass<ULyraExtendedMovementComponent>(ACharacter::CharacterMovementComponentName))
{

}

bool ALyraExtendedCharacter::CanJumpInternal_Implementation() const
{
	return Super::CanJumpInternal_Implementation() && !IsClimbing();
}

void ALyraExtendedCharacter::Climb()
{
	if (!CanClimb())
	{
		return;
	}

	FLedgeData LedgeData;
	if (!TryFindLedge(OUT LedgeData))
	{
		return;
	}

	StartClimbing(LedgeData);
	Server_StartClimbing(LedgeData);
}

void ALyraExtendedCharacter::Server_StartClimbing_Implementation(const FLedgeData& LedgeData)
{
	Multicast_StartClimbing(LedgeData);
}

void ALyraExtendedCharacter::Multicast_StartClimbing_Implementation(const FLedgeData& LedgeData)
{
	if (IsLocallyControlled())
	{
		return;
	}

	StartClimbing(LedgeData);
}

void ALyraExtendedCharacter::StartClimbing(const FLedgeData& LedgeData)
{
	ClimbingMovementData.StartLocation = GetActorLocation();
	ClimbingMovementData.StartRotation = GetActorRotation();
	ClimbingMovementData.EndLocation = LedgeData.Location + ClimbingEndLocationZOffset;
	ClimbingMovementData.EndRotation = LedgeData.Rotation;
	float LedgeHeight = ClimbingMovementData.EndLocation.Z - ClimbingMovementData.StartLocation.Z;
	const FClimbingOption& ClimbingOption = LedgeHeight >= HighClimbingOptionHeightThreshold ? HighClimbingOption : LowClimbingOption;
	ClimbingMovementData.Duration = ClimbingOption.Duration;
	ClimbingMovementData.Curve = ClimbingOption.Curve;
	ClimbingMovementData.StartAnimationLocation = ClimbingMovementData.EndLocation + ClimbingOption.StartingOffset.Z * FVector::DownVector + ClimbingOption.StartingOffset.Y * LedgeData.Normal;
	ClimbingMovementData.TimeOffset = FMath::GetMappedRangeValueClamped<float, float>({ ClimbingOption.LowestHeight, ClimbingOption.HighestHeight }, { ClimbingOption.LowestTimeOffset, ClimbingOption.HighestTimeOffset }, LedgeHeight);

	check(GetMesh()->GetAnimInstance());
	GetMesh()->GetAnimInstance()->Montage_Play(ClimbingOption.Montage, 1.0f, EMontagePlayReturnType::MontageLength, ClimbingMovementData.TimeOffset);

	GetCharacterMovement()->SetMovementMode(MOVE_Custom);
	bIsClimbing = true;

	GetWorld()->GetTimerManager().SetTimer(ClimbingTimer, this, &ALyraExtendedCharacter::StopClimbing, ClimbingMovementData.Duration - ClimbingMovementData.TimeOffset, false);
}

void ALyraExtendedCharacter::StopClimbing()
{
	GetCharacterMovement()->SetMovementMode(MOVE_Walking);
	bIsClimbing = false;
}

bool ALyraExtendedCharacter::CanClimb() const
{
	return GetExtendedMovementComponent()->IsMovingOnGround() && !IsClimbing();
}

bool ALyraExtendedCharacter::TryFindLedge(OUT FLedgeData& LedgeData) const
{
	FCollisionQueryParams CollisionQueryParams;
	CollisionQueryParams.AddIgnoredActor(this);
	CollisionQueryParams.bTraceComplex = true;

	FVector SelfBottom = GetActorLocation() + GetCapsuleComponent()->GetScaledCapsuleHalfHeight() * FVector::DownVector;

	// Forward sweep
	float ForwardSweepCapsuleRadius = GetCapsuleComponent()->GetScaledCapsuleRadius();
	float ForwardSweepCapsuleHalfHeight = (MaxLedgeHeight - MinLedgeHeight) / 2;
	FCollisionShape ForwardSweepCapsuleShape = FCollisionShape::MakeCapsule(ForwardSweepCapsuleRadius, ForwardSweepCapsuleHalfHeight);
	FVector ForwardSweepStartLocation = SelfBottom + (MinLedgeHeight + ForwardSweepCapsuleHalfHeight) * FVector::UpVector;
	FVector ForwardSweepEndLocation = ForwardSweepStartLocation + MaxDistToLedge * GetActorForwardVector();
	FHitResult ForwardSweepHitResult;
	bool bForwardSweepResult = GetWorld()->SweepSingleByChannel(ForwardSweepHitResult, ForwardSweepStartLocation, ForwardSweepEndLocation, FQuat::Identity, ECC_Climbing, ForwardSweepCapsuleShape, CollisionQueryParams);
#if ENABLE_DRAW_DEBUG
	if (DebugCVars::ShouldDrawClimbingDebug)
	{
		if (bForwardSweepResult)
		{
			DrawDebugCapsule(GetWorld(), ForwardSweepHitResult.Location, ForwardSweepCapsuleHalfHeight, ForwardSweepCapsuleRadius, FQuat::Identity, FColor::Blue, false, DebugCVars::ClimbingDebugLifetime);
		}
	}
#endif
	if (!bForwardSweepResult)
	{
		return false;
	}

	// Downward sweep
	float DownwardSweepSphereRadius = GetCapsuleComponent()->GetScaledCapsuleRadius();
	FCollisionShape DownwardSweepSphereShape = FCollisionShape::MakeSphere(DownwardSweepSphereRadius);
	constexpr float kDownwardSweepInsideOffset = 10.0f;
	FVector DownwardSweepStartLocation = ForwardSweepHitResult.ImpactPoint - kDownwardSweepInsideOffset * ForwardSweepHitResult.ImpactNormal;
	DownwardSweepStartLocation.Z = SelfBottom.Z + MaxLedgeHeight + DownwardSweepSphereRadius;
	FVector DownwardSweepEndLocation = DownwardSweepStartLocation;
	DownwardSweepEndLocation.Z = SelfBottom.Z;
	FHitResult DownwardSweepHitResult;
	bool bDownwardSweepResult = GetWorld()->SweepSingleByChannel(DownwardSweepHitResult, DownwardSweepStartLocation, DownwardSweepEndLocation, FQuat::Identity, ECC_Climbing, DownwardSweepSphereShape, CollisionQueryParams);
#if ENABLE_DRAW_DEBUG
	if (DebugCVars::ShouldDrawClimbingDebug)
	{
		if (bDownwardSweepResult)
		{
			DrawDebugSphere(GetWorld(), DownwardSweepHitResult.Location, DownwardSweepSphereRadius, 16, FColor::Blue, false, DebugCVars::ClimbingDebugLifetime);
		}
	}
#endif
	if (!bDownwardSweepResult)
	{
		return false;
	}

	// Overlap check
	float OverlapCapsuleRadius = GetCapsuleComponent()->GetScaledCapsuleRadius();
	float OverlapCapsuleHalfHeight = GetCapsuleComponent()->GetScaledCapsuleHalfHeight();
	FCollisionShape OverlapCapsuleShape = FCollisionShape::MakeCapsule(OverlapCapsuleRadius, OverlapCapsuleHalfHeight);
	constexpr float kOverlapZDelta = 1.0f;
	FVector OverlapLocation = DownwardSweepHitResult.ImpactPoint + (OverlapCapsuleHalfHeight + kOverlapZDelta) * FVector::UpVector;
	bool bOverlapResult = GetWorld()->OverlapBlockingTestByProfile(OverlapLocation, FQuat::Identity, NAME_LyraCharacterCollisionProfile_Capsule, OverlapCapsuleShape, CollisionQueryParams);
#if ENABLE_DRAW_DEBUG
	if (DebugCVars::ShouldDrawClimbingDebug)
	{
		DrawDebugCapsule(GetWorld(), OverlapLocation, OverlapCapsuleHalfHeight, OverlapCapsuleRadius, FQuat::Identity, bOverlapResult ? FColor::Red : FColor::Green, false, DebugCVars::ClimbingDebugLifetime);
	}
#endif
	if (bOverlapResult)
	{
		return false;
	}

	LedgeData.Location = OverlapLocation;
	LedgeData.Normal = ForwardSweepHitResult.ImpactNormal.GetSafeNormal2D();
	LedgeData.Rotation = (ForwardSweepHitResult.ImpactNormal * FVector(-1.0f, -1.0f, 0.0f)).ToOrientationRotator();

	return true;
}
