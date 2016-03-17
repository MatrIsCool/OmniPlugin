#pragma once

#include "OmniPluginPrivatePCH.h"
#include "OmniDataDelegate.h"
#include "OmniSingleController.h"
#include "OmniComponent.h"

#include "IOmniPlugin.h"

//FKey declarations
//Define each FKey const in a .cpp so we can compile
const FKey EKeysOmni::OmniLeftStylus("OmniLeftStylus");
const FKey EKeysOmni::OmniLeftExtra("OmniLeftExtra");

const FKey EKeysOmni::OmniLeftDocked("OmniLeftDocked");

const FKey EKeysOmni::OmniLeftMotionX("OmniLeftMotionX");
const FKey EKeysOmni::OmniLeftMotionY("OmniLeftMotionY");
const FKey EKeysOmni::OmniLeftMotionZ("OmniLeftMotionZ");

const FKey EKeysOmni::OmniLeftRotationPitch("OmniLeftRotationPitch");
const FKey EKeysOmni::OmniLeftRotationYaw("OmniLeftRotationYaw");
const FKey EKeysOmni::OmniLeftRotationRoll("OmniLeftRotationRoll");


const FKey EKeysOmni::OmniRightStylus("OmniRightStylus");
const FKey EKeysOmni::OmniRightExtra("OmniRightExtra");

const FKey EKeysOmni::OmniRightDocked("OmniRightDocked");

const FKey EKeysOmni::OmniRightMotionX("OmniRightMotionX");
const FKey EKeysOmni::OmniRightMotionY("OmniRightMotionY");
const FKey EKeysOmni::OmniRightMotionZ("OmniRightMotionZ");

const FKey EKeysOmni::OmniRightRotationPitch("OmniRightRotationPitch");
const FKey EKeysOmni::OmniRightRotationYaw("OmniRightRotationYaw");
const FKey EKeysOmni::OmniRightRotationRoll("OmniRightRotationRoll");

/** Empty Event Functions, no Super call required, because they don't do anything. Kept for easy future extension */
void OmniDataDelegate::OmniControllerEnabled(int32 controller){}
void OmniDataDelegate::OmniControllerDisabled(int32 controller){}

//Except the ones that are hooked into multi-cast delegates, which call each attached delegate with the given function via a lambda pass
void OmniDataDelegate::OmniPluggedIn()
{
	CallFunctionOnDelegates([](UOmniPluginComponent* delegate)
	{
		delegate->PluggedIn.Broadcast();
	});
}
void OmniDataDelegate::OmniUnplugged()
{
	CallFunctionOnDelegates([](UOmniPluginComponent* delegate)
	{
		delegate->Unplugged.Broadcast();
	});
}
void OmniDataDelegate::OmniDocked(int32 controllerId)
{
	UOmniSingleController* controller = OmniControllerForID(controllerId);
	CallFunctionOnDelegates([&](UOmniPluginComponent* delegate)
	{
		//Specialized function handles the broadcast
		delegate->Docked(controller);
	});
}
void OmniDataDelegate::OmniUndocked(int32 controllerId)
{
	UOmniSingleController* controller = OmniControllerForID(controllerId);
	CallFunctionOnDelegates([&](UOmniPluginComponent* delegate)
	{
		//Specialized function handles the broadcast
		delegate->Undocked(controller);
	});
}

void OmniDataDelegate::OmniButtonPressed(int32 controllerId, EOmniControllerButton button)
{
	UOmniSingleController* controller = OmniControllerForID(controllerId);
	CallFunctionOnDelegates([&](UOmniPluginComponent* delegate)
	{
		delegate->ButtonPressed.Broadcast(controller, button);
	});
}
void OmniDataDelegate::OmniButtonReleased(int32 controllerId, EOmniControllerButton button)
{
	UOmniSingleController* controller = OmniControllerForID(controllerId);
	CallFunctionOnDelegates([&](UOmniPluginComponent* delegate)
	{
		delegate->ButtonReleased.Broadcast(controller, button);
	});
}
void OmniDataDelegate::OmniStylusPressed(int32 controllerId){}
void OmniDataDelegate::OmniStylusReleased(int32 controllerId){}
void OmniDataDelegate::OmniExtraPressed(int32 controllerId){}
void OmniDataDelegate::OmniExtraReleased(int32 controllerId){}

void OmniDataDelegate::OmniControllerMoved(int32 controllerId,
	FVector position, FVector velocity, FVector acceleration,
	FRotator rotation, FRotator angularVelocity)
{
	UOmniSingleController* controller = OmniControllerForID(controllerId);
	CallFunctionOnDelegates([&](UOmniPluginComponent* delegate)
	{
		delegate->ControllerMoved.Broadcast(controller,position,velocity,acceleration,rotation,angularVelocity);
	});
};

/** Availability */
bool OmniDataDelegate::OmniIsAvailable()
{
	//TODO: Properly return the number of available device(s)
	return OmniLatestData->enabledCount == 1;

	//if both devices available :: return 2
}


void OmniDataDelegate::AddEventDelegate(UOmniPluginComponent* delegate)
{
	eventDelegates.Add(delegate);
}

void OmniDataDelegate::RemoveEventDelegate(UOmniPluginComponent* delegate)
{
	eventDelegates.Remove(delegate);
}


void OmniDataDelegate::CallFunctionOnDelegates(TFunction< void(UOmniPluginComponent*)> InFunction)
{
	for (UOmniPluginComponent* eventDelegate : eventDelegates)
	{
		InFunction(eventDelegate);
	}
}

OmniDataDelegate::OmniDataDelegate()
{
	//Ugly Init, but ensures no instance leakage, if you know how to remove the root references and attach this to a plugin parent UObject, make a pull request!
	LeftController = NewObject<UOmniSingleController>();
	LeftController->AddToRoot();	//we root these otherwise they can be removed early
	RightController = NewObject<UOmniSingleController>();
	RightController->AddToRoot();
}
OmniDataDelegate::~OmniDataDelegate(){
	//Cleanup root references
	if (LeftController)
		LeftController->RemoveFromRoot();
	if (RightController)
		RightController->RemoveFromRoot();
}

/** Call to determine which hand you're holding the controller in. Determine by last docking position.*/
EOmniControllerHand OmniDataDelegate::OmniWhichHand(int32 controller)
{
	return (EOmniControllerHand)OmniLatestData->controllers[controller].which_hand;
}


UOmniSingleController* OmniDataDelegate::OmniControllerForID(int32 controllerId)
{
	return OmniControllerForOmniHand(OmniWhichHand(controllerId));
}

UOmniSingleController* OmniDataDelegate::OmniControllerForControllerHand(EControllerHand hand)
{
	switch (hand)
	{
	case EControllerHand::Left:
		return LeftController;
		break;
	case EControllerHand::Right:
		return RightController;
		break;
	}
	return nullptr;
}

UOmniSingleController* OmniDataDelegate::OmniControllerForOmniHand(EOmniControllerHand hand)
{
	switch (hand){
	case OMNI_HAND_LEFT:
		return LeftController;
		break;
	case OMNI_HAND_RIGHT:
		return RightController;
		break;
	case OMNI_HAND_UNKNOWN:
		return nullptr;
		break;
	}
	return nullptr;
}

/** Poll for latest data.*/

hapticsControllerDataUE* OmniDataDelegate::OmniGetLatestData(int32 controllerId)
{
	if ((controllerId > MAX_CONTROLLERS_SUPPORTED) || (controllerId < 0))
	{
		return NULL; 
	}
	return OmniGetHistoricalData(controllerId, 0);
}

/** Poll for historical data. Valid index is 0-9 */
hapticsControllerDataUE* OmniDataDelegate::OmniGetHistoricalData(int32 controllerId, int32 historyIndex)
{
	if ((historyIndex<0) || (historyIndex>9) || (controllerId > MAX_CONTROLLERS_SUPPORTED) || (controllerId < 0))
	{
		return NULL;
	}
	hapticsControllerDataUE* data;
	data = &OmniHistoryData[historyIndex].controllers[controllerId];

	return data;
}

void OmniDataDelegate::UpdateControllerReference(hapticsControllerDataUE* controller, int index)
{
	//Get the hand index and set
	EOmniControllerHand hand = OmniWhichHand(index);

	if (hand == OMNI_HAND_LEFT)
	{
		LeftController->setFromHapticsDataUE(controller);
		LeftControllerId = index;
	}
	else if (hand == OMNI_HAND_RIGHT)
	{
		RightController->setFromHapticsDataUE(controller);
		RightControllerId = index;
	}
}

