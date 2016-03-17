#pragma once

#include "OmniEnum.h"
#include "IMotionController.h"

class UOmniSingleController;
class UOmniPluginComponent;

class OmniDataDelegate
{
	friend class FOmniController;
public:
	OmniDataDelegate();
	~OmniDataDelegate();
	
	//Namespace Omni for variables, functions and events.

	//Controller short-hands
	UOmniSingleController* LeftController = nullptr;
	int32 LeftControllerId = 1;
	UOmniSingleController* RightController = nullptr;
	int32 RightControllerId = 0;

	//Offset to base
	FVector baseOffset = FVector(0, 0, 0);
	
	//Event multi-cast delegation
	TArray<UOmniPluginComponent*> eventDelegates;
	void AddEventDelegate(UOmniPluginComponent* delegate);
	void RemoveEventDelegate(UOmniPluginComponent* delegate);

	void CallFunctionOnDelegates(TFunction< void(UOmniPluginComponent*)> InFunction);
	
	/** Latest will always contain the freshest controller data, external pointer do not delete*/
	hapticsAllControllerDataUE* OmniLatestData;

	/** Holds last 10 controller captures, useful for gesture recognition, external pointer do not delete*/
	hapticsAllControllerDataUE* OmniHistoryData;	//dynamic array size 10

	/** Event Emitters, override to receive notifications.
	 *	int32 controller is the controller index (typically 0 or 1 for Omni) 
	 *	Call OmniWhichHand(controller index) to determine which hand is being held (determined and reset on docking)
	 */
	virtual void OmniPluggedIn();								//called once enabledCount == 1
	virtual void OmniUnplugged();								//called once enabledCount == 0
	virtual void OmniControllerEnabled(int32 controller);		//called for each controller
	virtual void OmniControllerDisabled(int32 controller);		//called for each controller

	virtual void OmniDocked(int32 controller);
	virtual void OmniUndocked(int32 controller);

	virtual void OmniButtonPressed(int32 controller, EOmniControllerButton button);
	virtual void OmniButtonReleased(int32 controller, EOmniControllerButton button);
	virtual void OmniStylusPressed(int32 controller);
	virtual void OmniStylusReleased(int32 controller);
	virtual void OmniExtraPressed(int32 controller);
	virtual void OmniExtraReleased(int32 controller);
	
	/*	Movement converted into ue space (cm units in x,y,z and degrees in rotation) */
	virtual void OmniControllerMoved(int32 controller,						
								FVector position, FVector velocity, FVector acceleration, 
								FRotator rotation, FRotator angularVelocity);

	//** Callable Functions (for public polling support) */
	virtual bool OmniIsAvailable();
	virtual EOmniControllerHand OmniWhichHand(int32 controllerId);	//call to determine which hand the controller is held in. Determined and reset on controller docking.
	virtual UOmniSingleController* OmniControllerForID(int32 controllerId);
	virtual UOmniSingleController* OmniControllerForControllerHand(EControllerHand hand);
	virtual UOmniSingleController* OmniControllerForOmniHand(EOmniControllerHand hand);
	virtual hapticsControllerDataUE* OmniGetLatestData(int32 controllerId);
	virtual hapticsControllerDataUE* OmniGetHistoricalData(int32 controllerId, int32 historyIndex);

	void UpdateControllerReference(hapticsControllerDataUE* controller, int index);
};