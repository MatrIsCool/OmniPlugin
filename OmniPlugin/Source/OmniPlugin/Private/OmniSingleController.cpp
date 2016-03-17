#include "OmniPluginPrivatePCH.h"
#include "OmniSingleController.h"
#include <HD/hd.h>

UOmniSingleController::UOmniSingleController(const FObjectInitializer &init) : UObject(init)
{
}

bool UOmniSingleController::isLeftHand()
{
	return handPossession == EOmniControllerHand::OMNI_HAND_LEFT;
}

bool UOmniSingleController::isRightHand()
{
	return handPossession == EOmniControllerHand::OMNI_HAND_RIGHT;
}

void UOmniSingleController::setFromHapticsDataUE(hapticsControllerDataUE* data)
{
	this->position = data->position;
	//this->rawPosition = data->rawPosition;
	this->velocity = data->velocity;
	this->acceleration = data->acceleration;

	this->rawJointEncoder = data->raw_encoder;
	this->rawGimbalEncoder = data->gimbal_encoder;
	this->jointAngle = data->joint_angles;
	this->gimbalAngle = data->gimbal_angles;
	
	//this->orientation = data->rotation;
	this->angularVelocity = data->angular_velocity;

	this->stylusPressed = (data->buttons & HD_DEVICE_BUTTON_1) == HD_DEVICE_BUTTON_1;
	this->extraPressed = (data->buttons & HD_DEVICE_BUTTON_2) == HD_DEVICE_BUTTON_2;
	
	this->docked = data->is_docked;

	this->handPossession = (EOmniControllerHand)data->which_hand;
}