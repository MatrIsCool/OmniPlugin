#pragma once

#include "OmniPluginPrivatePCH.h"
#include "OmniEnum.h"
#include "OmniComponent.generated.h"

class OmniDataDelegate;

//These macros cannot be multi-line or it will not compile
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOmniPluggedInSignature);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOmniUnPluggedSignature);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOmniDockedSignature, class UOmniSingleController*, controller);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOmniUnDockedSignature, class UOmniSingleController*, controller);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOmniButtonPressedSignature, class UOmniSingleController*, controller, EOmniControllerButton, button);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOmniButtonReleasedSignature, class UOmniSingleController*, controller, EOmniControllerButton, button);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_SixParams(FOmniControllerMovedSignature, class UOmniSingleController*, controller, FVector, position, FVector, velocity, FVector, acceleration, FRotator, orientation, FRotator, angularVelocity);

UCLASS(ClassGroup="Input Controller", meta=(BlueprintSpawnableComponent))
class UOmniPluginComponent : public UActorComponent
{
	GENERATED_UCLASS_BODY()
public:

	//Assignable Events, used for fine-grained control
	//Buttons are relegated to input mapping, non-button features are available here

	UPROPERTY(BlueprintAssignable, Category = "Omni Events")
	FOmniPluggedInSignature PluggedIn;

	UPROPERTY(BlueprintAssignable, Category = "Omni Events")
	FOmniUnPluggedSignature Unplugged;

	UPROPERTY(BlueprintAssignable, Category = "Omni Events")
	FOmniDockedSignature ControllerDocked;

	UPROPERTY(BlueprintAssignable, Category = "Omni Events")
	FOmniUnDockedSignature ControllerUndocked;

	UPROPERTY(BlueprintAssignable, Category = "Omni Events")
	FOmniButtonPressedSignature ButtonPressed;

	UPROPERTY(BlueprintAssignable, Category = "Omni Events")
	FOmniButtonReleasedSignature ButtonReleased;

	UPROPERTY(BlueprintAssignable, Category = "Omni Events")
	FOmniControllerMovedSignature ControllerMoved;

	// Requires SetMeshComponentLinks to be called prior (e.g. in BeginPlay), then if true these will auto-hide when docked
	UPROPERTY(EditAnywhere, Category = "Omni Properties")
	bool HideMeshComponentsWhenDocked;

	// Once set, HideStaticMeshWhenDocked=true property will cause the bound components to hide.
	UFUNCTION(BlueprintCallable, Category = "Omni Functions")
	void SetMeshComponentLinks(UMeshComponent* LeftMesh, UMeshComponent* RightMesh);

	//Due to custom logic, call this instead of multi-cast directly
	void Docked(UOmniSingleController* controller);
	void Undocked(UOmniSingleController* controller);

	//Callable Blueprint functions - Need to be defined for direct access
	/** Check if the Omni is available/plugged in.*/
	UFUNCTION(BlueprintCallable, Category = OmniFunctions)
	bool IsAvailable();

	//** Poll for historical data.  Valid Hand is Left or Right, Valid history index is 0-9.  */
	UFUNCTION(BlueprintCallable, Category = OmniFunctions)
	UOmniSingleController* GetHistoricalFrameForHand(EOmniControllerHand hand = OMNI_HAND_LEFT, int32 historyIndex = 0);

	//** Get the latest available data given in a single frame. Valid Hand is Left or Right  */
	UFUNCTION(BlueprintCallable, Category = OmniFunctions)
	UOmniSingleController* GetLatestFrameForHand(EOmniControllerHand hand = OMNI_HAND_LEFT);

	/*
	// Set a manual offset, use this for manual calibration
	UFUNCTION(BlueprintCallable, Category = OmniFunctions)
	void SetBaseOffset(FVector Offset);

	// Use in-built calibration. Expects either a T-Pose. If offset is provided it will add the given offset to the final calibration.
	// For T-pose the function defaults to 40cm height. At 0,0,0 this will simply calibrate the zero position
	UFUNCTION(BlueprintCallable, Category = OmniFunctions)
	void Calibrate(FVector OffsetFromShoulderMidPoint = FVector(0,0,40));
	*/

	void SetDataDelegate(OmniDataDelegate* delegate);


protected:
	UMeshComponent* LeftMeshComponent;
	UMeshComponent* RightMeshComponent;

	//Utility Functions
	int32 ControllerIdForHand(EOmniControllerHand hand);

	/** Poll for historical data. Valid ControllerId is 0 or 1, Valid history index is 0-9.*/
	UOmniSingleController* GetHistoricalFrameForControllerId(int32 controllerId, int32 historyIndex);

	//** Get the latest available data given in a single frame. Valid ControllerId is 0 or 1  */
	UOmniSingleController* GetLatestFrameForControllerId(int32 controllerId);

	class OmniDataDelegate* dataDelegate;

	virtual void InitializeComponent() override;
	virtual void UninitializeComponent() override;
};