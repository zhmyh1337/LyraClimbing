// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Character/LyraCharacter.h"
#include "LyraExtendedCharacter.generated.h"

class ULyraExtendedMovementComponent;

USTRUCT()
struct FLedgeData
{
	GENERATED_BODY()

	UPROPERTY()
	FVector Location;

	UPROPERTY()
	FVector Normal;

	UPROPERTY()
	FRotator Rotation;
};

struct FClimbingMovementData
{
	FVector StartAnimationLocation;

	FVector StartLocation;
	FRotator StartRotation;

	FVector EndLocation;
	FRotator EndRotation;

	float Duration;
	float TimeOffset;

	class UCurveVector* Curve;
};

USTRUCT(BlueprintType)
struct FClimbingOption
{
	GENERATED_BODY()

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly)
	class UAnimMontage* Montage;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly)
	class UCurveVector* Curve;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, meta = (ClampMin = 0.0f, UIMin = 0.0f))
	float Duration;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, meta = (ClampMin = 0.0f, UIMin = 0.0f))
	float LowestHeight;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, meta = (ClampMin = 0.0f, UIMin = 0.0f))
	float HighestHeight;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, meta = (ClampMin = 0.0f, UIMin = 0.0f))
	float LowestTimeOffset;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, meta = (ClampMin = 0.0f, UIMin = 0.0f))
	float HighestTimeOffset;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly)
	FVector StartingOffset;
};

/**
 * 
 */
UCLASS()
class LYRAGAME_API ALyraExtendedCharacter : public ALyraCharacter
{
	GENERATED_BODY()
	
public:
	ALyraExtendedCharacter(const FObjectInitializer& ObjectInitializer);

	FORCEINLINE_DEBUGGABLE ULyraExtendedMovementComponent* GetExtendedMovementComponent() const
	{
		return StaticCast<ULyraExtendedMovementComponent*>(GetCharacterMovement());
	}

	virtual bool CanJumpInternal_Implementation() const;

    UFUNCTION(BlueprintCallable)
	void Climb();

	bool CanClimb() const;

	bool IsClimbing() const { return bIsClimbing; }

	const FClimbingMovementData& GetClimbingMovementData() const { return ClimbingMovementData; }

	const FTimerHandle& GetClimbingTimer() const { return ClimbingTimer; }

protected:
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Climbing|Trace", meta = (UIMin = 0.0f, ClampMin = 0.0f))
	float MaxDistToLedge = 50.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Climbing|Trace", meta = (UIMin = 0.0f, ClampMin = 0.0f))
	float MinLedgeHeight = 50.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Climbing|Trace", meta = (UIMin = 0.0f, ClampMin = 0.0f))
	float MaxLedgeHeight = 200.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Climbing", meta = (UIMin = 0.0f, ClampMin = 0.0f))
	float HighClimbingOptionHeightThreshold = 125.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Climbing", meta = (ClampMin = 0.0f, UIMin = 0.0f))
	float ClimbingEndLocationZOffset = 0.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Climbing")
	FClimbingOption HighClimbingOption;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Climbing")
	FClimbingOption LowClimbingOption;

	bool bIsClimbing = false;

	FClimbingMovementData ClimbingMovementData;

	FTimerHandle ClimbingTimer;

	bool TryFindLedge(OUT FLedgeData& LedgeData) const;

	void StartClimbing(const FLedgeData& LedgeData);

	void StopClimbing();

	UFUNCTION(Server, Reliable)
	void Server_StartClimbing(const FLedgeData& LedgeData);

	UFUNCTION(NetMulticast, Reliable)
	void Multicast_StartClimbing(const FLedgeData& LedgeData);
};
