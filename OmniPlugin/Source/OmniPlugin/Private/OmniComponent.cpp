#include "OmniPluginPrivatePCH.h"
#include "OmniComponent.h"
#include "OmniSingleController.h"
#include "IOmniPlugin.h"
#include "OmniDataDelegate.h"
#include "Engine.h"
#include "CoreUObject.h"

UOmniPluginComponent::UOmniPluginComponent(const FObjectInitializer &init) : UActorComponent(init)
{
	bWantsInitializeComponent = true;
	bAutoActivate = true;
}

void UOmniPluginComponent::InitializeComponent()
{
	Super::InitializeComponent();

	//Attach delegate references
	//dataDelegate = IOmniPlugin::Get().DataDelegate();
	//dataDelegate->AddEventDelegate(this);
	IOmniPlugin::Get().DeferedAddDelegate(this);
}

void UOmniPluginComponent::SetDataDelegate(OmniDataDelegate* data) {
	dataDelegate = data;
}


void UOmniPluginComponent::UninitializeComponent()
{
	//remove ourselves from the delegates
	dataDelegate->RemoveEventDelegate(this);
	dataDelegate = nullptr;

	Super::UninitializeComponent();
}

void UOmniPluginComponent::SetMeshComponentLinks(UMeshComponent* PassedLeftMesh, UMeshComponent* PassedRightMesh)
{
	LeftMeshComponent = PassedLeftMesh;
	RightMeshComponent = PassedRightMesh;
}

void UOmniPluginComponent::Docked(UOmniSingleController* controller)
{
	//Check possession and auto-hide if enabled
	if (HideMeshComponentsWhenDocked)
	{
		switch (controller->handPossession)
		{
		case OMNI_HAND_LEFT:
			if (LeftMeshComponent != nullptr)
				LeftMeshComponent->SetHiddenInGame(true);
			break;
		case OMNI_HAND_RIGHT:
			if (RightMeshComponent != nullptr)
				RightMeshComponent->SetHiddenInGame(true);
			break;
		default:
			break;
		}
	}

	//Emit our multi-cast delegate
	ControllerDocked.Broadcast(controller);
}

void UOmniPluginComponent::Undocked(UOmniSingleController* controller)
{
	//Check possession and auto-hide if enabled
	if (HideMeshComponentsWhenDocked){

		switch (controller->handPossession)
		{
		case OMNI_HAND_LEFT:
			if (LeftMeshComponent != nullptr)
				LeftMeshComponent->SetHiddenInGame(false);
			break;
		case OMNI_HAND_RIGHT:
			if (RightMeshComponent != nullptr)
				RightMeshComponent->SetHiddenInGame(false);
			break;
		default:
			break;
		}
	}

	//Emit our multi-cast delegate
	ControllerUndocked.Broadcast(controller);
}

//Utility Functions
bool UOmniPluginComponent::IsAvailable()
{
	return IOmniPlugin::IsAvailable();
}

UOmniSingleController* UOmniPluginComponent::GetHistoricalFrameForControllerId(int32 controllerId, int32 historyIndex)
{
	hapticsControllerDataUE* dataUE = dataDelegate->OmniGetHistoricalData(controllerId, historyIndex);

	UOmniSingleController* controller = NewObject<UOmniSingleController>(UOmniSingleController::StaticClass());
	controller->setFromHapticsDataUE(dataUE);
	return controller;
	return nullptr;
}

UOmniSingleController* UOmniPluginComponent::GetLatestFrameForControllerId(int32 controllerId)
{
	return GetHistoricalFrameForControllerId(controllerId, 0);
}

//Public Implementation
//Frames
UOmniSingleController* UOmniPluginComponent::GetHistoricalFrameForHand(EOmniControllerHand hand, int32 historyIndex)
{
	return GetHistoricalFrameForControllerId(ControllerIdForHand(hand), historyIndex);
}

UOmniSingleController* UOmniPluginComponent::GetLatestFrameForHand(EOmniControllerHand hand)
{
	return GetHistoricalFrameForHand(hand, 0);
}

/*
//Calibration
void UOmniPluginComponent::SetBaseOffset(FVector Offset)
{
	dataDelegate->baseOffset = Offset;
}

void UOmniPluginComponent::Calibrate(FVector OffsetFromShouldMidPoint)
{
	//Get left and right hand midpoint position
	dataDelegate->baseOffset = -(dataDelegate->LeftController->rawPosition + dataDelegate->RightController->rawPosition) / 2 + OffsetFromShouldMidPoint;
}
*/

//Determining Hand
int32 UOmniPluginComponent::ControllerIdForHand(EOmniControllerHand hand)
{
	switch (hand)
	{
	case OMNI_HAND_LEFT:
		return dataDelegate->LeftControllerId;
		break;
	case OMNI_HAND_RIGHT:
		return dataDelegate->RightControllerId;
		break;
	default:
		return 0;	//default to left
		break;
	}	
}


