// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Character/LyraCharacterMovementComponent.h"
#include "LyraExtendedMovementComponent.generated.h"

class ALyraExtendedCharacter;

/**
 * 
 */
UCLASS()
class LYRAGAME_API ULyraExtendedMovementComponent : public ULyraCharacterMovementComponent
{
	GENERATED_BODY()

public:
	FORCEINLINE_DEBUGGABLE ALyraExtendedCharacter* GetLyraExtendedCharacter() const
	{
		return StaticCast<ALyraExtendedCharacter*>(GetCharacterOwner());
	}

protected:
	virtual void PhysCustom(float deltaTime, int32 Iterations) override;
};
