#pragma once

#define MAX_CONTROLLERS_SUPPORTED 2
#define CONTROLLERS_PER_PLAYER	2

UENUM(BlueprintType)
enum EOmniControllerHand
{
	OMNI_HAND_UNKNOWN,
	OMNI_HAND_LEFT,
	OMNI_HAND_RIGHT
};

UENUM(BlueprintType)
enum EOmniControllerButton
{
	OMNI_BUTTON_STYLUS,
	OMNI_BUTTON_EXTRA
};

//Input Mapping Keys
struct EKeysOmni
{
	//Left Keys
	static const FKey OmniLeftStylus;
	static const FKey OmniLeftExtra;

	static const FKey OmniLeftDocked;

	static const FKey OmniLeftMotionX;
	static const FKey OmniLeftMotionY;
	static const FKey OmniLeftMotionZ;

	static const FKey OmniLeftRotationPitch;
	static const FKey OmniLeftRotationYaw;
	static const FKey OmniLeftRotationRoll;

	//Right Keys
	static const FKey OmniRightStylus;
	static const FKey OmniRightExtra;

	static const FKey OmniRightDocked;

	static const FKey OmniRightMotionX;
	static const FKey OmniRightMotionY;
	static const FKey OmniRightMotionZ;

	static const FKey OmniRightRotationPitch;
	static const FKey OmniRightRotationYaw;
	static const FKey OmniRightRotationRoll;
};

/** 
 * Converted Controller Data.
 * Contains converted raw and integrated 
 * Omni controller data.
 */
typedef struct _hapticsControllerDataUE{

	int32 OmniIndex;
	FVector position;			//converted to cm
	//FVector rawPosition;		//in cm without base offset
	//FQuat quat;				//converted to UE space, rotation in quaternion format
	//FRotator rotation;		//converted to UE space, use this version in blueprint
	FVector velocity;			//cm/s
	uint32 buttons;
	FString device_model;
	bool enabled;
	bool is_docked;
	uint8 which_hand;

	//raw values from device
	FVector raw_encoder;			//raw values
	FVector gimbal_encoder;			//raw values
	FRotator joint_angles;			//converted to deg
	FRotator gimbal_angles;			//converted to deg

	//calculated values
	FVector acceleration;			//cm/s^2
	FRotator angular_velocity;		//deg/s

} hapticsControllerDataUE;

typedef struct _hapticsAllControllerDataUE{
	hapticsControllerDataUE controllers[2];
	int32 enabledCount;
	bool available;
}hapticsAllControllerDataUE;