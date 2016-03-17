#include "OmniPluginPrivatePCH.h"

#include "IOmniPlugin.h"
#include "IMotionController.h"

#include "OmniDataDelegate.h"
#include "OmniSingleController.h"
#include "SlateBasics.h"
#include "IPluginManager.h"
#include "OmniComponent.h"

#include <iostream>
#include <stdexcept>
#include <vector>
#include <HD/hd.h>		//OpenHaptics
#include <windows.h>

#define LOCTEXT_NAMESPACE "OmniPlugin"
#define PLUGIN_VERSION "0.1"
DEFINE_LOG_CATEGORY_STATIC(OmniPluginLog, Log, All);

//Private API - This is where the magic happens

//DLL import definition
typedef int(*dll_hdInitDevice)(const char*);
typedef void(*dll_hdDisableDevice)(unsigned int);

typedef void(*dll_hdMakeCurrentDevice)(unsigned int);
typedef int(*dll_hdGetCurrentDevice)(void);

typedef void(*dll_hdBeginFrame)(unsigned int);
typedef void(*dll_hdEndFrame)(unsigned int);

typedef HDErrorInfo(*dll_hdGetError)(void);

typedef void(*dll_hdEnable)(unsigned int);
typedef void(*dll_hdDisable)(unsigned int);
typedef bool(*dll_hdIsEnabled)(unsigned int);

typedef void(*dll_hdGetBooleanv)(unsigned int, bool*);
typedef void(*dll_hdGetIntegerv)(unsigned int, unsigned int*);
typedef void(*dll_hdGetFloatv)(unsigned int, float*);
typedef void(*dll_hdGetDoublev)(unsigned int, double*);
typedef void(*dll_hdGetLongv)(unsigned int, long*);
typedef const char*(*dll_hdGetString)(unsigned int);

typedef void(*dll_hdSetBooleanv)(unsigned int, bool*);
typedef void(*dll_hdSetIntegerv)(unsigned int, unsigned int*);
typedef void(*dll_hdSetFloatv)(unsigned int, float*);
typedef void(*dll_hdSetDoublev)(unsigned int, double*);
typedef void(*dll_hdSetLongv)(unsigned int, long*);

//typedef unsigned int(*dll_hdCheckCalibration)(void);
//typedef unsigned int(*dll_hdCheckCalibrationStyle)(void);

typedef void(*dll_hdStartScheduler)(void);
typedef void(*dll_hdStopScheduler)(void);
typedef void(*dll_hdUnschedule)(unsigned long);

//typedef void(*dll_hdScheduleSynchronous)(unsigned int, unsigned int, unsigned short);
//typedef unsigned long(*dll_hdScheduleAsynchronous)(unsigned int, unsigned int, unsigned short);


dll_hdInitDevice OmniInit;
dll_hdDisableDevice OmniExit;

dll_hdMakeCurrentDevice OmniMakeCurrent;
dll_hdGetCurrentDevice OmniGetCurrent;

dll_hdBeginFrame OmniBeginFrame;
dll_hdEndFrame OmniEndFrame;

dll_hdGetError OmniGetError;

dll_hdEnable OmniEnable;
dll_hdDisable OmniDisable;
dll_hdIsEnabled OmniIsEnabled;

dll_hdGetBooleanv OmniGetBooleanv;
dll_hdGetIntegerv OmniGetIntegerv;
dll_hdGetFloatv OmniGetFloatv;
dll_hdGetDoublev OmniGetDoublev;
dll_hdGetLongv OmniGetLongv;
dll_hdGetString OmniGetString;

dll_hdSetBooleanv OmniSetBooleanv;
dll_hdSetIntegerv OmniSetIntegerv;
dll_hdSetFloatv OmniSetFloatv;
dll_hdSetDoublev OmniSetDoublev;
dll_hdSetLongv OmniSetLongv;

//dll_hdCheckCalibration OmniCheckCalibration;
//dll_hdCheckCalibrationStyle OmniCheckCalibrationStyle;

dll_hdStartScheduler OmniStartScheduler;
dll_hdStopScheduler OmniStopScheduler;
dll_hdUnschedule OmniUnschedule;

//dll_hdScheduleSynchronous OmniScheduleSynchronous;
//dll_hdScheduleAsynchronous OmniScheduleAsynchronous;


//UE v4.6 IM event wrappers
bool EmitKeyUpEventForKey(FKey key, int32 user, bool repeat)
{
	FKeyEvent KeyEvent(key, FSlateApplication::Get().GetModifierKeys(), user, repeat, 0, 0);
	return FSlateApplication::Get().ProcessKeyUpEvent(KeyEvent);
}

bool EmitKeyDownEventForKey(FKey key, int32 user, bool repeat)
{
	FKeyEvent KeyEvent(key, FSlateApplication::Get().GetModifierKeys(), user, repeat, 0, 0);
	return FSlateApplication::Get().ProcessKeyDownEvent(KeyEvent);
}

bool EmitAnalogInputEventForKey(FKey key, float value, int32 user, bool repeat)
{
	FAnalogInputEvent AnalogInputEvent(key, FSlateApplication::Get().GetModifierKeys(), user, repeat, 0, 0, value);
	return FSlateApplication::Get().ProcessAnalogInputEvent(AnalogInputEvent);
}

//Collector class contains all the data captured from .dll and delegate data will point to this structure (allDataUE and historicalDataUE).
class DataCollector
{
public:
	DataCollector()
	{
		allDataUE = new hapticsAllControllerDataUE;
		historicalDataUE = new hapticsAllControllerDataUE[10];

		for (int i = 0; i < 2; i++)
		{
			memset(&allDataUE->controllers[i], 0, sizeof(hapticsControllerDataUE));

			allDataUE->controllers[i].OmniIndex = i;

			allDataUE->controllers[i].position = FVector::ZeroVector;
			//allDataUE->controllers[i].rawPosition = FVector::ZeroVector;
			//allDataUE->controllers[i].quat = FQuat(0.0f, 0.0f, 0.0f, 0.0f);
			//allDataUE->controllers[i].rotation = FRotator::ZeroRotator;

			allDataUE->controllers[i].raw_encoder = FVector::ZeroVector;
			allDataUE->controllers[i].gimbal_encoder = FVector::ZeroVector;
			allDataUE->controllers[i].joint_angles = FRotator::ZeroRotator;
			allDataUE->controllers[i].gimbal_angles = FRotator::ZeroRotator;

			allDataUE->controllers[i].buttons = 0;

			allDataUE->controllers[i].enabled = false;
			allDataUE->enabledCount = 0;
			allDataUE->available = false;
		}
	}
	~DataCollector()
	{
		delete allDataUE;
		delete historicalDataUE;
	}

	hapticsAllControllerDataUE* allDataUE;
	hapticsAllControllerDataUE* historicalDataUE;
};


class FOmniController : public IInputDevice, public IMotionController
{

public:
	DataCollector *collector;
	OmniDataDelegate* OmniDelegate;
	void* DLLHandle;

	/** handler to send all messages to */
	TSharedRef<FGenericApplicationMessageHandler> MessageHandler;


	//Init and Runtime
	FOmniController(const TSharedRef< FGenericApplicationMessageHandler >& InMessageHandler)
		: MessageHandler(InMessageHandler)
	{
		UE_LOG(OmniPluginLog, Log, TEXT("Attempting to startup 'Phantom Omni' Module, v%s"), TEXT(PLUGIN_VERSION));

		collector = new DataCollector;
		OmniDelegate = new OmniDataDelegate;
		OmniDelegate->OmniLatestData = collector->allDataUE;	//set the delegate latest data pointer
		OmniDelegate->OmniHistoryData = collector->historicalDataUE;

		//Define Paths for direct dll bind
		FString BinariesRoot = FPaths::Combine(*FPaths::GameDir(), TEXT("Binaries"));
		FString PluginRoot = IPluginManager::Get().FindPlugin("OmniPlugin")->GetBaseDir();
		FString PlatformString;
		FString hdDLLString;

#if PLATFORM_WINDOWS
#if _WIN64
		//64bit
		hdDLLString = FString(TEXT("hd.dll"));
		PlatformString = FString(TEXT("Win64"));
#else
		//32bit
		hdDLLString = FString(TEXT("hd.dll"));
		PlatformString = FString(TEXT("Win32"));
#endif
#else
		UE_LOG(LogClass, Error, TEXT("Unsupported Platform. Omni Plugin Unavailable."));
#endif

		FString DllFilepath = FPaths::ConvertRelativePathToFull(FPaths::Combine(*PluginRoot, TEXT("ThirdParty/OpenHaptics/lib"), *hdDLLString));

		UE_LOG(OmniPluginLog, Log, TEXT("Fetching dll from %s"), *DllFilepath);

		//Check if the file exists, if not, give a detailed log entry why
		if (!FPaths::FileExists(DllFilepath)) {
			UE_LOG(OmniPluginLog, Error, TEXT("%s File is missing (Did you copy Binaries into project root?)! Omni Plugin Unavailable."), *hdDLLString);
			return;
		}

		DLLHandle = NULL;
		DLLHandle = FPlatformProcess::GetDllHandle(*DllFilepath);

		if (!DLLHandle) {
			UE_LOG(OmniPluginLog, Error, TEXT("GetDllHandle failed, 'Phantom Omni' Plugin Unavailable."));
			UE_LOG(OmniPluginLog, Error, TEXT("Full path debug: %s."), *DllFilepath);
			return;
		}

		HHD hHD1 = HD_INVALID_HANDLE;
		HHD hHD2 = HD_INVALID_HANDLE;

		OmniInit = NULL;
		OmniInit = (dll_hdInitDevice)FPlatformProcess::GetDllExport(DLLHandle, TEXT("hdInitDevice"));

		if (OmniInit != NULL)
		{
			UE_LOG(OmniPluginLog, Log, TEXT("[1] Attemp to Initialize 'Phantom Omni' Plugin..."));
			hHD1 = OmniInit(HD_DEFAULT_DEVICE);
		}

		//check if the device(s) is initialized properly before binding the rest of the functions.
		if (hHD1 == HD_INVALID_HANDLE)
		{
			UE_LOG(OmniPluginLog, Error, TEXT("[1]\tInitialize Error!, 'Phantom Omni' Plugin Unavailable."));
			collector->allDataUE->available = false;
			return;
		}
		else 
		{
			collector->allDataUE->available = true;

			UE_LOG(OmniPluginLog, Log, TEXT("[1]\tInitialized Success! 'Phantom Omni' Plugin Available."));
			collector->allDataUE->enabledCount = 1;

			//Attach all Right EKeys
			EKeys::AddKey(FKeyDetails(EKeysOmni::OmniRightStylus, LOCTEXT("OmniRightStylus", "Omni Right Stylus"), FKeyDetails::GamepadKey));
			EKeys::AddKey(FKeyDetails(EKeysOmni::OmniRightExtra, LOCTEXT("OmniRightExtra", "Omni Right Extra"), FKeyDetails::GamepadKey));

			EKeys::AddKey(FKeyDetails(EKeysOmni::OmniRightDocked, LOCTEXT("OmniRightDocked", "Omni Right Docked"), FKeyDetails::GamepadKey));

			EKeys::AddKey(FKeyDetails(EKeysOmni::OmniRightMotionX, LOCTEXT("OmniRightMotionX", "Omni Right Motion X"), FKeyDetails::FloatAxis));
			EKeys::AddKey(FKeyDetails(EKeysOmni::OmniRightMotionY, LOCTEXT("OmniRightMotionY", "Omni Right Motion Y"), FKeyDetails::FloatAxis));
			EKeys::AddKey(FKeyDetails(EKeysOmni::OmniRightMotionZ, LOCTEXT("OmniRightMotionZ", "Omni Right Motion Z"), FKeyDetails::FloatAxis));

			EKeys::AddKey(FKeyDetails(EKeysOmni::OmniRightRotationPitch, LOCTEXT("OmniRightRotationPitch", "Omni Right Rotation Pitch"), FKeyDetails::FloatAxis));
			EKeys::AddKey(FKeyDetails(EKeysOmni::OmniRightRotationYaw, LOCTEXT("OmniRightRotationYaw", "Omni Right Rotation Yaw"), FKeyDetails::FloatAxis));
			EKeys::AddKey(FKeyDetails(EKeysOmni::OmniRightRotationRoll, LOCTEXT("OmniRightRotationRoll", "Omni Right Rotation Roll"), FKeyDetails::FloatAxis));

			UE_LOG(OmniPluginLog, Log, TEXT("[1]\t\tAttached all 'Omni Right' Keys"));

			
			UE_LOG(OmniPluginLog, Log, TEXT("[1]\tAttemps to initialize 'Phantom_Left'..."));
			
			//Attemps to Init Phantom_Left (optional)
			hHD2 = OmniInit("Phantom_Left");

			if (hHD2 == HD_INVALID_HANDLE)
			{
				UE_LOG(OmniPluginLog, Error, TEXT("\t\thdInit(Left) Error!, Only 1 Phantom Omni device (Phantom_Right) available."));
			}
			else
			{
				UE_LOG(OmniPluginLog, Log, TEXT("[1]\t\tPhantom_Left available too."));
				collector->allDataUE->enabledCount = 2;

				//Left
				EKeys::AddKey(FKeyDetails(EKeysOmni::OmniLeftStylus, LOCTEXT("OmniLeftStylus", "Omni Left Stylus"), FKeyDetails::GamepadKey));
				EKeys::AddKey(FKeyDetails(EKeysOmni::OmniLeftExtra, LOCTEXT("OmniLeftExtra", "Omni Left Extra"), FKeyDetails::GamepadKey));

				EKeys::AddKey(FKeyDetails(EKeysOmni::OmniLeftMotionX, LOCTEXT("OmniLeftMotionX", "Omni Left Motion X"), FKeyDetails::FloatAxis));
				EKeys::AddKey(FKeyDetails(EKeysOmni::OmniLeftMotionY, LOCTEXT("OmniLeftMotionY", "Omni Left Motion Y"), FKeyDetails::FloatAxis));
				EKeys::AddKey(FKeyDetails(EKeysOmni::OmniLeftMotionZ, LOCTEXT("OmniLeftMotionZ", "Omni Left Motion Z"), FKeyDetails::FloatAxis));

				EKeys::AddKey(FKeyDetails(EKeysOmni::OmniLeftRotationPitch, LOCTEXT("OmniLeftRotationPitch", "Omni Left Rotation Pitch"), FKeyDetails::FloatAxis));
				EKeys::AddKey(FKeyDetails(EKeysOmni::OmniLeftRotationYaw, LOCTEXT("OmniLeftRotationYaw", "Omni Left Rotation Yaw"), FKeyDetails::FloatAxis));
				EKeys::AddKey(FKeyDetails(EKeysOmni::OmniLeftRotationRoll, LOCTEXT("OmniLeftRotationRoll", "Omni Left Rotation Roll"), FKeyDetails::FloatAxis));

				EKeys::AddKey(FKeyDetails(EKeysOmni::OmniLeftDocked, LOCTEXT("OmniLeftDocked", "Omni Left Docked"), FKeyDetails::GamepadKey));
			}
			

			//Binds the rest of the DLL
			OmniExit = (dll_hdDisableDevice)FPlatformProcess::GetDllExport(DLLHandle, TEXT("hdDisableDevice"));

			OmniMakeCurrent = (dll_hdMakeCurrentDevice)FPlatformProcess::GetDllExport(DLLHandle, TEXT("hdMakeCurrent"));
			OmniGetCurrent = (dll_hdGetCurrentDevice)FPlatformProcess::GetDllExport(DLLHandle, TEXT("hdGetCurrentDevice"));

			OmniBeginFrame = (dll_hdBeginFrame)FPlatformProcess::GetDllExport(DLLHandle, TEXT("hdBeginFrame"));
			OmniEndFrame = (dll_hdEndFrame)FPlatformProcess::GetDllExport(DLLHandle, TEXT("hdEndFrame"));

			OmniEnable = (dll_hdEnable)FPlatformProcess::GetDllExport(DLLHandle, TEXT("hdEnable"));
			OmniDisable = (dll_hdDisable)FPlatformProcess::GetDllExport(DLLHandle, TEXT("hdDisable"));
			OmniIsEnabled = (dll_hdIsEnabled)FPlatformProcess::GetDllExport(DLLHandle, TEXT("hdIsEnabled"));

			OmniGetBooleanv = (dll_hdGetBooleanv)FPlatformProcess::GetDllExport(DLLHandle, TEXT("hdGetBooleanv"));
			OmniGetIntegerv = (dll_hdGetIntegerv)FPlatformProcess::GetDllExport(DLLHandle, TEXT("hdGetIntegerv"));
			OmniGetFloatv = (dll_hdGetFloatv)FPlatformProcess::GetDllExport(DLLHandle, TEXT("hdGetFloatv"));
			OmniGetDoublev = (dll_hdGetDoublev)FPlatformProcess::GetDllExport(DLLHandle, TEXT("hdGetDoublev"));
			OmniGetLongv = (dll_hdGetLongv)FPlatformProcess::GetDllExport(DLLHandle, TEXT("hdGetLongv"));
			OmniGetString = (dll_hdGetString)FPlatformProcess::GetDllExport(DLLHandle, TEXT("hdGetString"));

			OmniSetBooleanv = (dll_hdSetBooleanv)FPlatformProcess::GetDllExport(DLLHandle, TEXT("hdSetBooleanv"));
			OmniSetIntegerv = (dll_hdSetIntegerv)FPlatformProcess::GetDllExport(DLLHandle, TEXT("hdSetIntegerv"));
			OmniSetFloatv = (dll_hdSetFloatv)FPlatformProcess::GetDllExport(DLLHandle, TEXT("hdSetFloatv"));
			OmniSetDoublev = (dll_hdSetDoublev)FPlatformProcess::GetDllExport(DLLHandle, TEXT("hdSetDoublev"));
			OmniSetLongv = (dll_hdSetLongv)FPlatformProcess::GetDllExport(DLLHandle, TEXT("hdSetLongv"));

			OmniStartScheduler = (dll_hdStartScheduler)FPlatformProcess::GetDllExport(DLLHandle, TEXT("hdStartScheduler"));
			OmniStopScheduler = (dll_hdStopScheduler)FPlatformProcess::GetDllExport(DLLHandle, TEXT("hdStopScheduler"));
			OmniUnschedule = (dll_hdUnschedule)FPlatformProcess::GetDllExport(DLLHandle, TEXT("hdUnschedule"));

			UE_LOG(OmniPluginLog, Log, TEXT("[1]\t\tBind the rest of the DLL functions successfully."));

			//Enable Force Output, Max Force Clamping, LED Status Light
			if (OmniEnable != NULL)
			{
				UE_LOG(OmniPluginLog, Log, TEXT("[2] Attemp to enable functionalities..."));
				OmniEnable(HD_FORCE_OUTPUT);
				OmniEnable(HD_MAX_FORCE_CLAMPING);
				OmniEnable(HD_USER_STATUS_LIGHT);
				UE_LOG(OmniPluginLog, Log, TEXT("[2]\tEnabled: HD_FORCE_OUTPUT, HD_MAX_FORCE_CLAMPING, HD_USER_STATUS_LIGHT."));
			}
			else {
				UE_LOG(OmniPluginLog, Log, TEXT("[2]\tOmniEnable is NULL!  Something's wrong."));
			}

			//Start servo loop scheduler
			if (OmniStartScheduler != NULL)
			{
				UE_LOG(OmniPluginLog, Log, TEXT("[3] Attemp to start Servo Loop..."));
				OmniStartScheduler();
				UE_LOG(OmniPluginLog, Log, TEXT("[3]\tStarted Servo Loop Scheduler successfully."));
			}
			else
			{
				UE_LOG(OmniPluginLog, Log, TEXT("[3]\tOmniStartScheduler is NULL!  Something's wrong."));
			}

		}

		//Required calls at init
		IModularFeatures::Get().RegisterModularFeature(GetModularFeatureName(), this);

		//@todo:  fix this.  construction of the controller happens after InitializeMotionControllers(), so we manually insert into the array here.
		GEngine->MotionControllerDevices.AddUnique(this);
	}

#undef LOCTEXT_NAMESPACE


	virtual ~FOmniController()
	{
		//Perform cleaning tasks ONLY IF the plugin successfully initialized.
		if (collector->allDataUE->available)
		{
			//Stop servo loop scheduler 
			OmniStopScheduler();
			UE_LOG(OmniPluginLog, Log, TEXT("[Exit] Servo Loop Scheduler stopped."));

			//Disable Omni device(s)... one by one.
			for (int i = 0; i < collector->allDataUE->enabledCount; i++)
			{
				OmniExit(OmniGetCurrent());
			}
			UE_LOG(OmniPluginLog, Log, TEXT("[Exit] Phantom Omni(s) disabled."));
		}

		FPlatformProcess::FreeDllHandle(DLLHandle);

		UE_LOG(OmniPluginLog, Log, TEXT("[Exit] Phantom Omni Plugin clean shutdown."));


		delete OmniDelegate;
		delete collector;

		GEngine->MotionControllerDevices.Remove(this);
	}

	virtual void Tick(float DeltaTime) override
	{
		//Update Data History
		DelegateUpdateAllData(DeltaTime);
		DelegateEventTick();	//does SendControllerEvents not get late sampled?
	}

	virtual void SendControllerEvents() override
	{
		//DelegateEventTick();	//does SendControllerEvents not get late sampled?
	}


	//Omni only supports one player so ControllerIndex is ignored.
	virtual bool GetControllerOrientationAndPosition(const int32 ControllerIndex, const EControllerHand DeviceHand, FRotator& OutOrientation, FVector& OutPosition) const
	{
		bool RetVal = false;

		UOmniSingleController* controller = OmniDelegate->OmniControllerForControllerHand(DeviceHand);

		if (controller != nullptr && !controller->docked)
		{
			OutOrientation = controller->gimbalAngle; //ORIGINAL : controller->rotation
			OutPosition = controller->position;
			RetVal = true;
		}

		return RetVal;
	}

	virtual bool Exec(UWorld* InWorld, const TCHAR* Cmd, FOutputDevice& Ar) override
	{
		return false;
	}

	void SetChannelValue(int32 UnrealControllerId, FForceFeedbackChannelType ChannelType, float Value) override
	{
	}
	void SetChannelValues(int32 UnrealControllerId, const FForceFeedbackValues& Values) override
	{
	}
	virtual void SetMessageHandler(const TSharedRef< FGenericApplicationMessageHandler >& InMessageHandler) override
	{
		MessageHandler = InMessageHandler;
	}

private:
	//Delegate Private functions
	void DelegateUpdateAllData(float DeltaTime);
	void DelegateCheckEnabledCount(bool* plugNotChecked);
	void DelegateEventTick();

};



//Public API Implementation


/** Delegate Functions, called by plugin to keep data in sync and to emit the events.*/
void FOmniController::DelegateUpdateAllData(float DeltaTime)
{
	//int nButtons = 0;

	//if the Omni(s) are unavailable don't try to get more information
	if (!collector->allDataUE->available) {
		UE_LOG(OmniPluginLog, Log, TEXT("Data Collector not available."));
		return;
	}

	/********************************************************************************/
	/*																				*/
	/*						  Get Data from Phantom Device(s)						*/
	/*																				*/
	/********************************************************************************/	
	
	
	//OmniBeginFrame(OmniGetCurrent());			// TODO: FIX THIS

	//Loop for each controller (Phantom_Right first)
	for (int i = 0; i < collector->allDataUE->enabledCount; i++) {

		//Begin Haptic Frame
		OmniBeginFrame(i);

		//Get current device index
		//collector->allDataUE->controllers[i].OmniIndex = OmniGetCurrent();
		collector->allDataUE->controllers[i].OmniIndex = i;
		collector->allDataUE->controllers[i].enabled = true;

		//Get device model type
		collector->allDataUE->controllers[i].device_model = OmniGetString(HD_DEVICE_MODEL_TYPE);

		//Set device hand (Right as a default)
		if (i == 0) {
			collector->allDataUE->controllers[i].which_hand = OMNI_HAND_RIGHT;
		} else {
			collector->allDataUE->controllers[i].which_hand = OMNI_HAND_LEFT;
		}

		//Get inkwell switch 
		bool isNotDocked = true;													// For some strange reasons, OpenHaptics wants Inkwell_Switch 
		OmniGetBooleanv(HD_CURRENT_INKWELL_SWITCH, &isNotDocked);					// to returns 0 if the device is currently docked... ???
		collector->allDataUE->controllers[i].is_docked = !isNotDocked;				// So, we flipped the value.

		//Get currently pressed button
		unsigned int buttons;
		OmniGetIntegerv(HD_CURRENT_BUTTONS, &buttons);
		collector->allDataUE->controllers[i].buttons = buttons;


		//		::Coordinate System::												//
		//																			//
		//				  : 	  X			  :		  Y		 :		  Z				//
		//	----------------------------------------------------------------------	//
		//	OpenHaptics   :		Right		  :		  Up	 :	 toward User		//
		//	Unreal Engine :	  toward User	  :		 Right   :		  Up			//
		//																			//


		//Get position
		double pos[16];
		OmniGetDoublev(HD_CURRENT_TRANSFORM, pos);									//mm
		collector->allDataUE->controllers[i].position.X = pos[12];
		collector->allDataUE->controllers[i].position.Y = pos[13];					// NOTE: From http://www.h3dapi.org/modules/newbb/viewtopic.php?topic_id=777&forum=6
		collector->allDataUE->controllers[i].position.Z = pos[14];					// TODO: convert the coordinate system properly

		//DEBUG info template
		//UE_LOG(OmniPluginLog, Log, TEXT("Position X = %F\n"), collector->allDataUE->controllers[0].position.X);
		//UE_LOG(OmniPluginLog, Log, TEXT("Position Y = %F\n"), collector->allDataUE->controllers[0].position.Y);
		//UE_LOG(OmniPluginLog, Log, TEXT("Position Z = %F\n"), collector->allDataUE->controllers[0].position.Z);

		//Get velocity
		double vel[3];
		OmniGetDoublev(HD_CURRENT_VELOCITY, vel);									//mm/s
		collector->allDataUE->controllers[i].velocity.X = vel[0];
		collector->allDataUE->controllers[i].velocity.Y = vel[1];
		collector->allDataUE->controllers[i].velocity.Z = vel[2];

		//Get raw encoder values
		long encoder[6];
		OmniGetLongv(HD_CURRENT_ENCODER_VALUES, encoder);
		collector->allDataUE->controllers[i].raw_encoder.X = encoder[0];
		collector->allDataUE->controllers[i].raw_encoder.Y = encoder[1];
		collector->allDataUE->controllers[i].raw_encoder.Z = encoder[2];
		collector->allDataUE->controllers[i].gimbal_encoder.X = encoder[3];
		collector->allDataUE->controllers[i].gimbal_encoder.Y = encoder[4];
		collector->allDataUE->controllers[i].gimbal_encoder.Z = encoder[5];

		//Get current joint angles
		double joint_angles[3];
		OmniGetDoublev(HD_CURRENT_JOINT_ANGLES, joint_angles);						//rad
		joint_angles[0] = (joint_angles[0] * 180) / PI;
		joint_angles[1] = (joint_angles[1] * 180) / PI;								//Convert to deg
		joint_angles[2] = (joint_angles[2] * 180) / PI;
		collector->allDataUE->controllers[i].joint_angles.Yaw = joint_angles[0];
		collector->allDataUE->controllers[i].joint_angles.Pitch = joint_angles[1];
		collector->allDataUE->controllers[i].joint_angles.Roll = joint_angles[2];

		//Get current gimbals angles (The Stylus)
		double gimbal_angles[3];
		OmniGetDoublev(HD_CURRENT_GIMBAL_ANGLES, gimbal_angles);					//rad
		gimbal_angles[0] = (gimbal_angles[0] * 180) / PI;
		gimbal_angles[1] = (gimbal_angles[1] * 180) / PI;							//Convert to deg
		gimbal_angles[2] = (gimbal_angles[2] * 180) / PI;
		collector->allDataUE->controllers[i].gimbal_angles.Yaw = gimbal_angles[0];
		collector->allDataUE->controllers[i].gimbal_angles.Pitch = gimbal_angles[1];
		collector->allDataUE->controllers[i].gimbal_angles.Roll = gimbal_angles[2];
	}

	//Update all historical links
	OmniDelegate->OmniHistoryData[9] = OmniDelegate->OmniHistoryData[8];
	OmniDelegate->OmniHistoryData[8] = OmniDelegate->OmniHistoryData[7];
	OmniDelegate->OmniHistoryData[7] = OmniDelegate->OmniHistoryData[6];
	OmniDelegate->OmniHistoryData[6] = OmniDelegate->OmniHistoryData[5];
	OmniDelegate->OmniHistoryData[5] = OmniDelegate->OmniHistoryData[4];
	OmniDelegate->OmniHistoryData[4] = OmniDelegate->OmniHistoryData[3];
	OmniDelegate->OmniHistoryData[3] = OmniDelegate->OmniHistoryData[2];
	OmniDelegate->OmniHistoryData[2] = OmniDelegate->OmniHistoryData[1];
	OmniDelegate->OmniHistoryData[1] = OmniDelegate->OmniHistoryData[0];

	//Add in all the integrated data (acceleration/angular velocity/etc.)
	hapticsControllerDataUE* controller;
	hapticsControllerDataUE* previous;

	for (int i = 0; i < collector->allDataUE->enabledCount; i++)
	{
		controller = &OmniDelegate->OmniLatestData->controllers[i];
		previous = &OmniDelegate->OmniHistoryData[0].controllers[i];

		//Calculate acceleration, and angular velocity
		controller->acceleration = (controller->velocity - previous->velocity) / DeltaTime;
		controller->angular_velocity = FRotator(controller->gimbal_angles - previous->gimbal_angles);
		controller->angular_velocity.Yaw /= DeltaTime;
		controller->angular_velocity.Pitch /= DeltaTime;
		controller->angular_velocity.Roll /= DeltaTime;
	}

	for (int i = 0; i < collector->allDataUE->enabledCount; i++) {
		//End Haptic Frame
		OmniEndFrame(i);
	}
	
	//OmniEndFrame(OmniGetCurrent());

	//Update the stored data with the integrated data obtained from latest
	OmniDelegate->OmniHistoryData[0] = *OmniDelegate->OmniLatestData;
	
}

void FOmniController::DelegateCheckEnabledCount(bool* plugNotChecked)
{
	if (!*plugNotChecked) return;

	hapticsAllControllerDataUE* previous = &OmniDelegate->OmniHistoryData[0];
	int32 oldCount = previous->enabledCount;
	int32 count = OmniDelegate->OmniLatestData->enabledCount;
	if (oldCount != count)
	{
		if (count == 1)	//Omni controller number, STEM behavior undefined.
		{
			OmniDelegate->OmniPluggedIn();
			*plugNotChecked = false;
		}
		else if (count == 0)
		{
			OmniDelegate->OmniUnplugged();
			*plugNotChecked = false;
		}
	}
}

//Function used for consistent conversion to input mapping basis
float MotionInputMappingConversion(float AxisValue) {
	return FMath::Clamp(AxisValue / 100.f, -1.f, 1.f);
}

/** Internal Tick - Called by the Plugin */
void FOmniController::DelegateEventTick()
{
	hapticsControllerDataUE* controller;
	hapticsControllerDataUE* previous;
	bool plugNotChecked = true;

	//Trigger any delegate events
	for (int i = 0; i < MAX_CONTROLLERS_SUPPORTED; i++)
	{
		controller = &OmniDelegate->OmniLatestData->controllers[i];
		previous = &OmniDelegate->OmniHistoryData[1].controllers[i];

		//Sync any extended data forms such as OmniSingleController, so that the correct instance values are sent and not 1 frame delayed
		OmniDelegate->UpdateControllerReference(controller, i);

		//If it is enabled run through all the event notifications and data integration
		if (controller->enabled)
		{
			//Enable Check
			if (controller->enabled != previous->enabled)
			{
				DelegateCheckEnabledCount(&plugNotChecked);
				OmniDelegate->OmniControllerEnabled(i);

				//Skip this loop, previous state comparisons will be wrong
				continue;
			}

			//Determine Hand to support dynamic input mapping
			bool leftHand = OmniDelegate->OmniWhichHand(i) == 1;

			//Docking
			if (controller->is_docked != previous->is_docked)
			{
				if (controller->is_docked)
				{
					OmniDelegate->OmniDocked(i);
					if (leftHand)
						EmitKeyDownEventForKey(EKeysOmni::OmniLeftDocked, 0, 0);
					else
						EmitKeyDownEventForKey(EKeysOmni::OmniRightDocked, 0, 0);
				}
				else {
					OmniDelegate->OmniUndocked(i);
					if (leftHand)
						EmitKeyUpEventForKey(EKeysOmni::OmniLeftDocked, 0, 0);
					else
						EmitKeyUpEventForKey(EKeysOmni::OmniRightDocked, 0, 0);
				}
			}

			//** Buttons */

			//B1
			if ((controller->buttons & HD_DEVICE_BUTTON_1) != (previous->buttons & HD_DEVICE_BUTTON_1))
			{
				if ((controller->buttons & HD_DEVICE_BUTTON_1) == HD_DEVICE_BUTTON_1)
				{
					OmniDelegate->OmniButtonPressed(i, OMNI_BUTTON_STYLUS);
					OmniDelegate->OmniStylusPressed(i);
					//InputMapping
					if (leftHand)
					{
						EmitKeyDownEventForKey(EKeysOmni::OmniLeftStylus, 0, 0);
						EmitKeyDownEventForKey(FGamepadKeyNames::MotionController_Left_FaceButton1, 0, 0);
					}
					else
					{
						EmitKeyDownEventForKey(EKeysOmni::OmniRightStylus, 0, 0);
						EmitKeyDownEventForKey(FGamepadKeyNames::MotionController_Right_FaceButton1, 0, 0);
					}
				}
				else {
					OmniDelegate->OmniButtonReleased(i, OMNI_BUTTON_STYLUS);
					OmniDelegate->OmniStylusReleased(i);
					//InputMapping
					if (leftHand)
					{
						EmitKeyUpEventForKey(EKeysOmni::OmniLeftStylus, 0, 0);
						EmitKeyUpEventForKey(FGamepadKeyNames::MotionController_Left_FaceButton1, 0, 0);
					}
					else
					{
						EmitKeyUpEventForKey(EKeysOmni::OmniRightStylus, 0, 0);
						EmitKeyUpEventForKey(FGamepadKeyNames::MotionController_Right_FaceButton1, 0, 0);
					}
				}
			}
			//B2
			if ((controller->buttons & HD_DEVICE_BUTTON_2) != (previous->buttons & HD_DEVICE_BUTTON_2))
			{
				if ((controller->buttons & HD_DEVICE_BUTTON_2) == HD_DEVICE_BUTTON_2)
				{
					OmniDelegate->OmniButtonPressed(i, OMNI_BUTTON_EXTRA);
					OmniDelegate->OmniExtraPressed(i);
					//InputMapping
					if (leftHand)
					{
						EmitKeyDownEventForKey(EKeysOmni::OmniLeftExtra, 0, 0);
						EmitKeyDownEventForKey(FGamepadKeyNames::MotionController_Left_FaceButton2, 0, 0);
					}
					else
					{
						EmitKeyDownEventForKey(EKeysOmni::OmniRightExtra, 0, 0);
						EmitKeyDownEventForKey(FGamepadKeyNames::MotionController_Right_FaceButton2, 0, 0);
					}
				}
				else {
					OmniDelegate->OmniButtonReleased(i, OMNI_BUTTON_EXTRA);
					OmniDelegate->OmniExtraReleased(i);
					//InputMapping
					if (leftHand)
					{
						EmitKeyUpEventForKey(EKeysOmni::OmniLeftExtra, 0, 0);
						EmitKeyUpEventForKey(FGamepadKeyNames::MotionController_Left_FaceButton2, 0, 0);
					}
					else
					{
						EmitKeyUpEventForKey(EKeysOmni::OmniRightExtra, 0, 0);
						EmitKeyUpEventForKey(FGamepadKeyNames::MotionController_Right_FaceButton2, 0, 0);
					}
				}
			}
			

			/** Movement */

			//Controller

			if (!controller->is_docked) {
				FRotator rotation = FRotator(controller->gimbal_angles); // ORIGINAL : controller->rotation

				//If the controller isn't docked, it's moving
				OmniDelegate->OmniControllerMoved(i,
					controller->position, controller->velocity, controller->acceleration,
					rotation, controller->angular_velocity);

				//InputMapping
				if (leftHand)
				{
					//2 meters = 1.0
					EmitAnalogInputEventForKey(EKeysOmni::OmniLeftMotionX, MotionInputMappingConversion(controller->position.X), 0, 0);
					EmitAnalogInputEventForKey(EKeysOmni::OmniLeftMotionY, MotionInputMappingConversion(controller->position.Y), 0, 0);
					EmitAnalogInputEventForKey(EKeysOmni::OmniLeftMotionZ, MotionInputMappingConversion(controller->position.Z), 0, 0);

					EmitAnalogInputEventForKey(EKeysOmni::OmniLeftRotationPitch, rotation.Pitch / 90.f, 0, 0);
					EmitAnalogInputEventForKey(EKeysOmni::OmniLeftRotationYaw, rotation.Yaw / 180.f, 0, 0);
					EmitAnalogInputEventForKey(EKeysOmni::OmniLeftRotationRoll, rotation.Roll / 180.f, 0, 0);
				}
				else
				{
					//2 meters = 1.0
					EmitAnalogInputEventForKey(EKeysOmni::OmniRightMotionX, MotionInputMappingConversion(controller->position.X), 0, 0);
					EmitAnalogInputEventForKey(EKeysOmni::OmniRightMotionY, MotionInputMappingConversion(controller->position.Y), 0, 0);
					EmitAnalogInputEventForKey(EKeysOmni::OmniRightMotionZ, MotionInputMappingConversion(controller->position.Z), 0, 0);

					EmitAnalogInputEventForKey(EKeysOmni::OmniRightRotationPitch, rotation.Pitch / 90.f, 0, 0);
					EmitAnalogInputEventForKey(EKeysOmni::OmniRightRotationYaw, rotation.Yaw / 180.f, 0, 0);
					EmitAnalogInputEventForKey(EKeysOmni::OmniRightRotationRoll, rotation.Roll / 180.f, 0, 0);
				}
			}
		}//end enabled
		else {
			if (controller->enabled != previous->enabled)
			{
				DelegateCheckEnabledCount(&plugNotChecked);
				OmniDelegate->OmniControllerDisabled(i);
			}
		}
	}//end controller for loop
}

//Implementing the module, required
class FOmniPlugin : public IOmniPlugin
{
	FOmniController* controllerReference = nullptr;
	TArray<UOmniPluginComponent*> delegateComponents;
	bool inputDeviceCreated = false;

	virtual TSharedPtr< class IInputDevice > CreateInputDevice(const TSharedRef< FGenericApplicationMessageHandler >& InMessageHandler) override
	{
		controllerReference = new FOmniController(InMessageHandler);
		// Add deferred delegates
		for (auto& actorComponent : delegateComponents) {
			controllerReference->OmniDelegate->AddEventDelegate(actorComponent);
			actorComponent->SetDataDelegate(controllerReference->OmniDelegate);
		}
		delegateComponents.Empty();
		inputDeviceCreated = true;

		return TSharedPtr< class IInputDevice >(controllerReference);
	}

	virtual OmniDataDelegate* DataDelegate() override
	{
		return controllerReference->OmniDelegate;
	}

	virtual void DeferedAddDelegate(UOmniPluginComponent* delegate) override
	{
		if (inputDeviceCreated)
		{
			controllerReference->OmniDelegate->AddEventDelegate(delegate);
			delegate->SetDataDelegate(controllerReference->OmniDelegate);
		}
		else
		{
			// defer until later
			delegateComponents.Add(delegate);
		}
	}

};

//Second parameter needs to be called the same as the Module name or packaging will fail
IMPLEMENT_MODULE(FOmniPlugin, OmniPlugin)