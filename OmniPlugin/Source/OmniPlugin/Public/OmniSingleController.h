#pragma once

#include "OmniEnum.h"
#include "OmniSingleController.generated.h"

UCLASS(BlueprintType)
class UOmniSingleController : public UObject
{
	GENERATED_UCLASS_BODY()
public:

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Omni Frame")
	int32 controllerId;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Omni Frame")
	FVector position;

	//UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Omni Frame")
	//FVector rawPosition;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Omni Frame")
	FVector velocity;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Omni Frame")
	FVector acceleration;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Omni Frame")
	FVector rawJointEncoder;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Omni Frame")
	FVector rawGimbalEncoder;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Omni Frame")
	FRotator jointAngle;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Omni Frame")
	FRotator gimbalAngle;

	//UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Omni Frame")
	//FRotator orientation;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Omni Frame")
	FRotator angularVelocity;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Omni Frame")
	bool stylusPressed;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Omni Frame")
	bool extraPressed;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Omni Frame")
	bool docked;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Omni Frame")
	TEnumAsByte<EOmniControllerHand> handPossession;

	//Convenience Call, optionally check hand possession property
	UFUNCTION(BlueprintCallable, Category = "Omni Frame")
	bool isLeftHand();

	//Convenience Call, optionally check hand possession property
	UFUNCTION(BlueprintCallable, Category = "Omni Frame")
	bool isRightHand();

	//Conversion
	void setFromHapticsDataUE(hapticsControllerDataUE* data);
};