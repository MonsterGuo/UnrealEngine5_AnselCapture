// Copyright Epic Games, Inc. All Rights Reserved.

#include "CoreMinimal.h"   
#include "Containers/Map.h"
#include "Containers/StaticBitArray.h"   
#include "IAnselPlugin.h"
#include "Camera/CameraTypes.h"
#include "Camera/CameraPhotography.h"
#include "Camera/PlayerCameraManager.h"
#include "HAL/ConsoleManager.h"
#include "Engine/Engine.h"
#include "Engine/GameViewportClient.h"
#include "Engine/LocalPlayer.h"
#include "Engine/ViewportSplitScreen.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/HUD.h"
#include "GameFramework/WorldSettings.h"
#include "Kismet/GameplayStatics.h"
#include "Widgets/SWindow.h"
#include "Application/SlateApplicationBase.h"
#include "RenderResource.h"
#include <Runtime/ApplicationCore/Public/Windows/WindowsApplication.h> 
#include <functional>
#include "Interfaces/IPluginManager.h"

#include "AnselCapture.h"
#include "Engine/Engine.h"
#include "EngineUtils.h"
#include "Kismet/GameplayStatics.h"
#include "AnselFunctionLibrary.h"
#include <AnselSDK.h>

DEFINE_LOG_CATEGORY_STATIC(LogAnsel, Log, All);

#define LOCTEXT_NAMESPACE "Photography"

//����̨��ز���
static TAutoConsoleVariable<int32> CVarPhotographyAllow(
	TEXT("r.Photography.Allow"),
	1,
	TEXT("If 1, allow the user to freeze the scene and potentially use a roaming camera to\n")
	TEXT("take screenshots.  Set this dynamically to permit or forbid photography per-level,\n")
	TEXT("per-cutscene, etc.  (Default: 1)"));

static TAutoConsoleVariable<int32> CVarPhotographyEnableMultipart(
	TEXT("r.Photography.EnableMultipart"),
	1,
	TEXT("If 1, allow the photography system to take high-resolution shots that need to be rendered in tiles which are later stitched together.  (Default: 1)"));

static TAutoConsoleVariable<int32> CVarPhotographySettleFrames(
	TEXT("r.Photography.SettleFrames"),
	10,
	TEXT("The number of frames to let the rendering 'settle' before taking a photo.  Useful to allow temporal AA/smoothing to work well; if not using any temporal effects, can be lowered for faster capture.  (Default: 10)"));

static TAutoConsoleVariable<float> CVarPhotographyTranslationSpeed(
	TEXT("r.Photography.TranslationSpeed"),
	300.0f,
	TEXT("Normal speed (in Unreal Units per second) at which to move the roaming photography camera. (Default: 300.0)"));

static TAutoConsoleVariable<float> CVarConstrainCameraSize(
	TEXT("r.Photography.Constrain.CameraSize"),
	14.0f,
	TEXT("Radius (in Unreal Units) of sphere around the camera; used to prevent the camera clipping into nearby geometry when constraining camera with collision.  Negative values disable default camera collisions. (Default: 14.0)"));

static TAutoConsoleVariable<float> CVarConstrainCameraDistance(
	TEXT("r.Photography.Constrain.MaxCameraDistance"),
	2500.0f,
	TEXT("Maximum distance (in Unreal Units) which camera is allowed to wander from its initial position when constraining camera by distance.  Negative values disable default distance contraints. (Default: 2500.0)"));

static TAutoConsoleVariable<int32> CVarPhotographyAutoPostprocess(
	TEXT("r.Photography.AutoPostprocess"),
	1,
	TEXT("If 1, the photography system will attempt to automatically disable HUD, subtitles, and some standard postprocessing effects during photography sessions/captures which are known to give poor photography results.  Set to 0 to manage all postprocessing tweaks manually from the PlayerCameraManager Blueprint callbacks.  Note: Blueprint callbacks will be called regardless of AutoPostprocess value.  (Default: auto-disable (1)"));

static TAutoConsoleVariable<int32> CVarPhotographyAutoPause(
	TEXT("r.Photography.AutoPause"),
	1,
	TEXT("If 1, the photography system will attempt to ensure that the level is paused while in photography mode.  Set to 0 to manage pausing and unpausing manually from the PlayerCameraManager Blueprint callbacks.    Note: Blueprint callbacks will be called regardless of AutoPause value.  (Default: auto-pause (1)"));

static TAutoConsoleVariable<int32> CVarAllowHighQuality(
	TEXT("r.Photography.AllowHighQuality"),
	1,
	TEXT("Whether to permit Ansel RT (high-quality mode).\n"),
	ECVF_RenderThreadSafe);

// intentionally undocumented
static TAutoConsoleVariable<int32> CVarExtreme(
	TEXT("r.Photography.Extreme"),
	0,
	TEXT("Whether to use 'extreme' quality settings for Ansel RT (EXPERIMENTAL).\n"),
	ECVF_RenderThreadSafe);

/////////////////////////////////////////////////
// All the Ansel-specific details

class FNVAnselCameraPhotographyPrivate : public ICameraPhotography
{
public:
	FNVAnselCameraPhotographyPrivate(); //���캯��
	virtual ~FNVAnselCameraPhotographyPrivate() override; //��������
	virtual bool UpdateCamera(FMinimalViewInfo& InOutPOV, APlayerCameraManager* PCMgr) override; // �������
	virtual void UpdatePostProcessing(FPostProcessSettings& InOutPostProcessSettings) override;	//���º���
	virtual void StartSession() override;	//��ʼ�Ự
	virtual void StopSession() override;	//�����Ự
	virtual bool IsSupported() override;	//�Ƿ�֧��
	virtual void SetUIControlVisibility(uint8 UIControlTarget, bool bIsVisible) override;	//����UI�ɼ���
	//Ĭ�ϰ��������
	virtual void DefaultConstrainCamera(const FVector NewCameraLocation, const FVector PreviousCameraLocation, const FVector OriginalCameraLocation, FVector& OutCameraLocation, APlayerCameraManager* PCMgr) override;
	//��ȡ�ṩ������
	virtual const TCHAR* const GetProviderName() override { return TEXT("NVIDIA Ansel"); };
	
	enum econtrols {
		control_dofscale,  //����
		control_dofsensorwidth,	//�����
		control_doffocalregion, //�ص�����
		control_doffocaldistance,	//����
		control_dofdepthbluramount, //���ģ����
		control_dofdepthblurradius, //���ģ���뾶
		control_bloomintensity,		//�ع�ǿ��
		control_bloomscale,			//�ع��С
		control_scenefringeintensity, //��������ǿ��
		control_COUNT				//������Ŀ
	};
	//���� union
	typedef union {
		bool bool_val; //boolֵ
		float float_val;//����ֵ
	} ansel_control_val;

private:
	void ReconfigureAnsel(); //��������
	void DeconfigureAnsel(); //ɾ������
	//��ʼ�Ự�Ļص�
	static ansel::StartSessionStatus AnselStartSessionCallback(ansel::SessionConfiguration& settings, void* userPointer);
	//ֹͣ�Ự�Ļص�
	static void AnselStopSessionCallback(void* userPointer);
	//��ʼץȡ�Ļص�
	static void AnselStartCaptureCallback(const ansel::CaptureConfiguration& CaptureInfo, void* userPointer);
	//ֹͣץȡ�Ļص�
	static void AnselStopCaptureCallback(void* userPointer);
	//���������Ļص�
	static void AnselChangeQualityCallback(bool isHighQuality, void* userPointer);
	//���ƥ��
	static bool AnselCamerasMatch(ansel::Camera& a, ansel::Camera& b);
	//Ansel�������С��ͼ
	void AnselCameraToFMinimalView(FMinimalViewInfo& InOutPOV, ansel::Camera& AnselCam);
	//��С��ͼ�� Ansel���
	void FMinimalViewToAnselCamera(ansel::Camera& InOutAnselCam, FMinimalViewInfo& POV);
	//��ͼ��������
	bool BlueprintModifyCamera(ansel::Camera& InOutAnselCam, APlayerCameraManager* PCMgr); // returns whether modified cam is in original (session-start) position
	
	//������Ⱦ���� ΪͼƬץȡ
	void ConfigureRenderingSettingsForPhotography(FPostProcessSettings& InOutPostProcessSettings);
	//��������Ự
	void SetUpSessionCVars();
	//�����Զ���UI����
	void DoCustomUIControls(FPostProcessSettings& InOutPPSettings, bool bRebuildControls);
	//��������
	void DeclareSlider(int id, FText LocTextLabel, float LowerBound, float UpperBound, float Val);
	//����UI����
	bool ProcessUISlider(int id, float& InOutVal);

	//ץȡֵ
	bool CaptureCVar(FString CVarName);
	//����ץȡֵ��ν�ʣ�ֵ���ƣ�ֵ���û���ã����� �Ƚϣ���Ҫ���ã�ʹ���������ȼ���
	void SetCapturedCVarPredicated(const char* CVarName, float valueIfNotReset, std::function<bool(const float, const float)> comparison, bool wantReset, bool useExistingPriority);
	//����ץȡֵ��ֵ���ƣ�ֵ��������ã���Ҫ����=�٣�ʹ����������ֵ=�٣�
	void SetCapturedCVar(const char* CVarName, float valueIfNotReset, bool wantReset = false, bool useExistingPriority = false);

	// ansel ������
	ansel::Configuration* AnselConfig;
	//ansel��� Ansel���
	ansel::Camera AnselCamera;
	//ansel��� Ansel���ԭ��
	ansel::Camera AnselCameraOriginal;
	//ansel��� ansel���Ԥ��
	ansel::Camera AnselCameraPrevious;

	//��С��ͼ��Ϣ UEԭ�����
	FMinimalViewInfo UECameraOriginal;
	//��С��ͼ��Ϣ UE���Ԥ��
	FMinimalViewInfo UECameraPrevious;
	//�������� UEԭ�еĺ�������
	FPostProcessSettings UEPostProcessingOriginal;

	bool bAnselSessionActive;						//ansel�Ự����
	bool bAnselSessionNewlyActive;					//anselh�Ự�µļ���
	bool bAnselSessionWantDeactivate;				//ansel�Ựȡ������
	bool bAnselCaptureActive;						//Anselץȡ����
	bool bAnselCaptureNewlyActive;					//Anselץȡ����
	bool bAnselCaptureNewlyFinished;				//Anselץȡ�µĽ���
	ansel::CaptureConfiguration AnselCaptureInfo;	//Ansel::ץȡ���� anselץȡ��Ϣ

	/******   ����    ********/
	//�Ƿ������һ�ֵĲɼ�
	bool bTriggerNextCapture;
	//�Ƿ񼤻���һ�ε�Tick
	bool bTriggerNextTick;

	//ǿ�� ��ֹ
	bool bForceDisallow;
	//�Ƿ� ǿ������ͶӰ
	bool bIsOrthoProjection;

	//�Ƿ�����ƶ���� �ڻỰ֮ǰ
	bool bWasMovableCameraBeforeSession;
	//�Ƿ���ͣ �ڻỰ֮ǰ
	bool bWasPausedBeforeSession;
	// �Ƿ���ʾHDR �ڻỰ֮ǰ
	bool bWasShowingHUDBeforeSession;
	// b�Ựǰ�Ƿ�������Ļ
	bool bWereSubtitlesEnabledBeforeSession;
	// �Ƿ�˥�����ûỰ֮ǰ
	bool bWasFadingEnabledBeforeSession;
	// �Ƿ�ӫĻ��Ϣ����Ự֮ǰ
	bool bWasScreenMessagesEnabledBeforeSession = false;
	// ʱ�����ͻỰ
	float fTimeDilationBeforeSession;
	// ����Ƿ���ԭ����״̬
	bool bCameraIsInOriginalState = true;

	//�Զ�����
	bool bAutoPostprocess;
	//�Զ���ͣ
	bool bAutoPause;
	//��׷�ǲ��Ǳ����� = ������
	bool bRayTracingEnabled = false;
	//�Ƿ�Ȳ�����ͣ =  ��
	bool bPausedInternally = false;
	//������ģʽ 
	bool bHighQualityModeDesired = false;
	//������ģʽ �Ѿ�����
	bool bHighQualityModeIsSetup = false;

	//ansel �� FOV���� = Ansel��ˮƽFOV
	ansel::FovType RequiredFovType = ansel::kHorizontalFov;
	//ansel �� Fov���� ��ǰ����Fov���� = ansel �� ˮƽFOV
	ansel::FovType CurrentlyConfiguredFovType = ansel::kHorizontalFov;
	// Ҫ�� ����ĵ�λ =100
	float RequiredWorldToMeters = 100.f;
	//��ǰ��������� ���� = 0
	float CurrentlyConfiguredWorldToMeters = 0.f;

	//�ԻỰ������֡��
	uint32_t NumFramesSinceSessionStart;

	// members relating to the 'Game Settings' controls in the Ansel overlay UI
	//��Ansel overlay UI�еġ���Ϸ���á��ؼ���صĳ�Ա
	TStaticBitArray<256> bEffectUIAllowed; //Ч��UI�Ƿ�����

	//UI������Ҫ���¹���
	bool bUIControlsNeedRebuild;
	//ansel �û����� UI���ơ����Ƶ���Ŀ�� ����һ������
	ansel::UserControlDesc UIControls[control_COUNT];
	// ansel���Ƶ�ֵ UI����ֵ�����顿
	static ansel_control_val UIControlValues[control_COUNT]; // static to allow access from a callback
	//UI���Ƶĵ�λ����
	float UIControlRangeLower[control_COUNT];
	//UI���Ƶĸ�λ����
	float UIControlRangeUpper[control_COUNT];

	/** Console variable delegate for checking when the console variables have changed */
	// ����̫�����Ĵ��� Ϊ��ѯ  �� ����̨����������
	FConsoleCommandDelegate CVarDelegate;  //ֵ�Ĵ���
	FConsoleVariableSinkHandle CVarDelegateHandle; //����̨������ �³� ���
	
	// ֵ����Ϣ 
	struct CVarInfo {
		//����̨����
		IConsoleVariable* cvar;
		//�����ֵ
		float fInitialVal;
	};
	//��ʼ��ֵ�� ��ֵ��
	TMap<FString, CVarInfo> InitialCVarMap;
};
//ansel����ֵ   Ansel���˽��� ����UI����ֵs��������Ŀ��
FNVAnselCameraPhotographyPrivate::ansel_control_val FNVAnselCameraPhotographyPrivate::UIControlValues[control_COUNT];
//AnselSDKDLL �ľ��ָ��
static void* AnselSDKDLLHandle = 0;
//Ansel��DLL�Ƿ�������
static bool bAnselDLLLoaded = false;

//NVAnsel���ͼ��ץȡ˽���ࣺ��ץȡֵ������ֵ̨�����ƣ�
bool FNVAnselCameraPhotographyPrivate::CaptureCVar(FString CVarName)
{
	//��ȡ����̨��ֵ��ָ��
	IConsoleVariable* cvar = IConsoleManager::Get().FindConsoleVariable(CVarName.GetCharArray().GetData());
	//��ָ�������ȥ
	if (!cvar) return false;
	//����ָ̨����Ϣ ����
	CVarInfo info;
	//��Ϣ��ֵ = ָ��ץȡ����ֵ
	info.cvar = cvar;
	//��Ϣ,��ʼ����ֵ = ��ֵת��Ϊ����ֵ
	info.fInitialVal = cvar->GetFloat();

	InitialCVarMap.Add(CVarName, info);
	return true;
}

// NVAnsel���ͼ��ץȡ˽���ࣺ�������Ĺ��캯������
FNVAnselCameraPhotographyPrivate::FNVAnselCameraPhotographyPrivate()
	: ICameraPhotography()		//�����Ĭ�Ϲ��캯��
	, bAnselSessionActive(false)  //Ĭ�ϵĲ�����
	, bAnselSessionNewlyActive(false)	//Ĭ�ϲ��������
	, bAnselSessionWantDeactivate(false)	//�Ự�ǲ�����Ҫȡ������
	, bAnselCaptureActive(false)		//Ansel��ץȡ�Ƿ񼤻��	
	, bAnselCaptureNewlyActive(false)	//Ansel��ץȡ�µļ����
	, bAnselCaptureNewlyFinished(false)	//Ansel��ץȡ�Ƿ��������
	, bTriggerNextCapture(false)
	, bTriggerNextTick(false)
	, bForceDisallow(false)				//ǿ�ƽ���
	, bIsOrthoProjection(false)			//�Ƿ�ǿ������ͶӰ����
{
	//����Ч��UI�Ƿ񼤻�
	for (int i = 0; i < bEffectUIAllowed.Num(); ++i)
	{
		//Ĭ��ȫȫ��Ϊ����
		bEffectUIAllowed[i] = true; // allow until explicitly disallowed �����ֱ����ȷ��ֹ
	}
	//���DLLû������
	if (bAnselDLLLoaded)
	{
		//�½�һ��Ansel������
		AnselConfig = new ansel::Configuration();
		//����ֵ̨�Ĵ��� 
		CVarDelegate = FConsoleCommandDelegate::CreateLambda([this] {
			//��һ�ε���ֵ�����¸�ֵ
			static float LastTranslationSpeed = -1.0f; //������Ϊ����������뽫֮ǰ�ļ�¼����
			static int32 LastSettleFrames = -1;
			//��һ�ε� �ƶ��ٶ� = �ӿ���̨ץȡ�������ƶ��ٶ�ֵ
			float ThisTranslationSpeed = CVarPhotographyTranslationSpeed->GetFloat();
			//��һ�εľ�̬֡�� = �ӿ���̨ץȡ�����ľ�̬֡��ֵ
			int32 ThisSettleFrames = CVarPhotographySettleFrames->GetInt();
			//��һ�ε��ƶ��ٶȡ���̬֡������һ�ε��ƶ���ֵ����̬֡������һ��
			if (ThisTranslationSpeed != LastTranslationSpeed ||
				ThisSettleFrames != LastSettleFrames)
			{
				//��������Ansel
				ReconfigureAnsel();
				//Ȼ��������ֵ���¸�ֵ
				LastTranslationSpeed = ThisTranslationSpeed;
				LastSettleFrames = ThisSettleFrames;
			}
		});
		//����ֵ̨�Ĵ���ľ�� 
		CVarDelegateHandle = IConsoleManager::Get().RegisterConsoleVariableSink_Handle(CVarDelegate);
		//�������ô���
		ReconfigureAnsel();
	}
	else
	{
		//���������Ansel��DLLû�гɹ��������
		UE_LOG(LogAnsel, Log, TEXT("Ansel DLL was not successfully loaded."));
	}
}

//��������
FNVAnselCameraPhotographyPrivate::~FNVAnselCameraPhotographyPrivate()
{	
	if (bAnselDLLLoaded)
	{
		IConsoleManager::Get().UnregisterConsoleVariableSink_Handle(CVarDelegateHandle);
		DeconfigureAnsel();
		//���ｫ��ɾ��Ansel�����ö���
		delete AnselConfig;
	}
}

//Ansel�Ƿ�֧��
bool FNVAnselCameraPhotographyPrivate::IsSupported()
{
	//����������ʶλ ���� Ansel�Ѿ�������û
	return bAnselDLLLoaded && ansel::isAnselAvailable();
}

//����UI���ƿɼ��ԣ�Ui���Ƶ�Ŀ�꣨ö�٣����Ƿ�ɼ���  //������ù����ǵ�������������˼
void FNVAnselCameraPhotographyPrivate::SetUIControlVisibility(uint8 UIControlTarget, bool bIsVisible)
{
	//��������þ������ö�Ӧ��UIЧ���Ƿ�ɼ�����
	bEffectUIAllowed[UIControlTarget] = bIsVisible;
}

//Ansel���ƥ�䣨�����b ƥ�䵽���a��
bool FNVAnselCameraPhotographyPrivate::AnselCamerasMatch(ansel::Camera& a, ansel::Camera& b)
{
	//��Ҫ�У�λ�ã���ת��fov��ӳ��ƫ�ƣ�
	return a.position.x == b.position.x &&
		a.position.y == b.position.y &&
		a.position.z == b.position.z &&
		a.rotation.x == b.rotation.x &&
		a.rotation.y == b.rotation.y &&
		a.rotation.z == b.rotation.z &&
		a.rotation.w == b.rotation.w &&
		a.fov == b.fov &&
		a.projectionOffsetX == b.projectionOffsetX &&
		a.projectionOffsetY == b.projectionOffsetY;
}

//Ansel����� ����С�Ӵ�����ת������������С�ӽ���Ϣ�� ��Ϊ������������Ansel�������
void FNVAnselCameraPhotographyPrivate::AnselCameraToFMinimalView(FMinimalViewInfo& InOutPOV, ansel::Camera& AnselCam)
{
	//Fovֱֵ�Ӹ�
	InOutPOV.FOV = AnselCam.fov;
	//����λ��ֱ�Ӹ�
	InOutPOV.Location.X = AnselCam.position.x;
	InOutPOV.Location.Y = AnselCam.position.y;
	InOutPOV.Location.Z = AnselCam.position.z;
	//��ת������һ��ת��
	FQuat rotq(AnselCam.rotation.x, AnselCam.rotation.y, AnselCam.rotation.z, AnselCam.rotation.w);
	//����ת��ļ����ֵ��rotq����ǿתΪFRotator��ֵ
	InOutPOV.Rotation = FRotator(rotq);
	//���������ƫ�ƺ�������ƫ�ƽ��и�ֵ��
	InOutPOV.OffCenterProjectionOffset.Set(AnselCam.projectionOffsetX, AnselCam.projectionOffsetY);
}

//С�Ӵ���Ansel��������������Ansel�������С�ӽ���Ϣ POV��
void FNVAnselCameraPhotographyPrivate::FMinimalViewToAnselCamera(ansel::Camera& InOutAnselCam, FMinimalViewInfo& POV)
{
	//Ansel���
	InOutAnselCam.fov = POV.FOV;
	//Anselλ��
	InOutAnselCam.position = { float(POV.Location.X), float(POV.Location.Y), float(POV.Location.Z) };
	//��ת��ת��
	FQuat rotq = POV.Rotation.Quaternion();
	InOutAnselCam.rotation = { float(rotq.X), float(rotq.Y), float(rotq.Z), float(rotq.W) };
	InOutAnselCam.projectionOffsetX = 0.f; // Ansel only writes these, doesn't read
	InOutAnselCam.projectionOffsetY = 0.f;
}

//��ͼ���������Ansel��������������� �����������
bool FNVAnselCameraPhotographyPrivate::BlueprintModifyCamera(ansel::Camera& InOutAnselCam, APlayerCameraManager* PCMgr)
{
	//��С�ӽ�  �����
	FMinimalViewInfo Proposed;
	//Ansel�������С�ӽǣ������ ������������
	AnselCameraToFMinimalView(Proposed, InOutAnselCam);
	//������������(������Ӵ�λ�ã����Ԥ��.λ�ã�UEԭ�����.λ�� �������λ��)
	PCMgr->PhotographyCameraModify(Proposed.Location, UECameraPrevious.Location, UECameraOriginal.Location, Proposed.Location/*out by ref*/);
	// only position has possibly changed
	// ����λ���Ѿ� ���ܸ���
	InOutAnselCam.position.x = Proposed.Location.X;
	InOutAnselCam.position.y = Proposed.Location.Y;
	InOutAnselCam.position.z = Proposed.Location.Z;

	//���Ԥ�� = �����
	UECameraPrevious = Proposed;

	//�Ƿ������ԭ����λ��
	bool bIsCameraInOriginalTransform =
		Proposed.Location.Equals(UECameraOriginal.Location) &&
		Proposed.Rotation.Equals(UECameraOriginal.Rotation) &&
		Proposed.FOV == UECameraOriginal.FOV;
	return bIsCameraInOriginalTransform;
}

//���� ������ID ,�������ֱ�ǩ����ͱ߽磬��߽߱磬ֵ ��
void FNVAnselCameraPhotographyPrivate::DeclareSlider(int id, FText LocTextLabel, float LowerBound, float UpperBound, float Val)
{
	//�߽߱�͵ͱ߽�
	UIControlRangeLower[id] = LowerBound;
	UIControlRangeUpper[id] = UpperBound;

	//UI����[id].��ǩ = �ַ����������ı���ǩ.���ַ�������ȡ�ַ������飬��ȡ���ݣ�
	UIControls[id].labelUtf8 = TCHAR_TO_UTF8(LocTextLabel.ToString().GetCharArray().GetData());
	//UI����ֵ�ġ�id��.����ֵ = 
	UIControlValues[id].float_val = FMath::GetRangePct(LowerBound, UpperBound, Val);
	//UI����ID.�ص� = [](�û�������Ϣ ��Ϣ)
	UIControls[id].callback = [](const ansel::UserControlInfo& info) {
		//UI����ֵ����Ϣ.��
		UIControlValues[info.userControlId - 1].float_val = *(float*)info.value;
	};
	//�û�Id = id+1
	UIControls[id].info.userControlId = id + 1; // reserve 0 as 'unused'
	//�û��������� = Ansel���û���������
	UIControls[id].info.userControlType = ansel::kUserControlSlider;
	//��Ϣ��ֵ = UI���Ƶ�ֵ������ֵ
	UIControls[id].info.value = &UIControlValues[id].float_val;
	//�û�����״̬  = Ansel������û�����״̬��UI���ơ�id����
	ansel::UserControlStatus status = ansel::addUserControl(UIControls[id]);
	UE_LOG(LogAnsel, Log, TEXT("control#%d status=%d"), (int)id, (int)status);
}

//���� UI ����
bool FNVAnselCameraPhotographyPrivate::ProcessUISlider(int id, float& InOutVal)
{
	//�����UI���ơ�id����Ϣ���û�����ID��
	if (UIControls[id].info.userControlId <= 0)
	{
		return false; // control is not in use
	}

	InOutVal = FMath::Lerp(UIControlRangeLower[id], UIControlRangeUpper[id], UIControlValues[id].float_val);
	return true;
}

//�����Զ���UI���ƣ��������� ���� �����¹������ƣ�
void FNVAnselCameraPhotographyPrivate::DoCustomUIControls(FPostProcessSettings& InOutPPSettings, bool bRebuildControls)
{
	if (bRebuildControls)
	{
		// clear existing controls ������ڵ�UI����
		for (int i = 0; i < control_COUNT; ++i)
		{
			if (UIControls[i].info.userControlId > 0) // we are using id 0 as 'unused'
			{
				//�Ƴ��û�����
				ansel::removeUserControl(UIControls[i].info.userControlId);
				//UI����ID����Ϣ.�û�����ID
				UIControls[i].info.userControlId = 0;
			}
		}

		// save postproc settings at session start
		//�ڻỰ��ʼʱ������������
		UEPostProcessingOriginal = InOutPPSettings;

		// add all relevant controls ������й�������
		PRAGMA_DISABLE_DEPRECATION_WARNINGS
		//�Ƿ�����ȳ�
		if (bEffectUIAllowed[DepthOfField])
		{
			const bool bAnyDofVisible = InOutPPSettings.DepthOfFieldFstop > 0 && InOutPPSettings.DepthOfFieldFocalDistance > 0;

			if (bAnyDofVisible)
			{
				DeclareSlider(
					control_dofsensorwidth,
					LOCTEXT("control_dofsensorwidth", "Focus Sensor"), // n.b. similar effect to focus scale
					0.1f, 1000.f,
					InOutPPSettings.DepthOfFieldSensorWidth
				);

				DeclareSlider(
					control_doffocaldistance,
					LOCTEXT("control_doffocaldistance", "Focus Distance"),
					0.f, 1000.f, // UU - doc'd to 10000U but that's too coarse for a narrow UI control
					InOutPPSettings.DepthOfFieldFocalDistance
				);

				DeclareSlider(
					control_dofdepthbluramount,
					LOCTEXT("control_dofbluramount", "Blur Distance km"),
					0.000001f, 1.f, // km; doc'd as up to 100km but that's too coarse for a narrow UI control
					InOutPPSettings.DepthOfFieldDepthBlurAmount
				);

				DeclareSlider(
					control_dofdepthblurradius,
					LOCTEXT("control_dofblurradius", "Blur Radius"),
					0.f, 4.f,
					InOutPPSettings.DepthOfFieldDepthBlurRadius
				);
			}
		}
		PRAGMA_ENABLE_DEPRECATION_WARNINGS
		//���Ч��������ع⡿
		if (bEffectUIAllowed[Bloom] &&
			InOutPPSettings.BloomIntensity > 0.f)
		{
			DeclareSlider(
				control_bloomintensity,
				LOCTEXT("control_bloomintensity", "Bloom Intensity"),//�ع�ǿ��  
				0.f, 8.f,
				InOutPPSettings.BloomIntensity
			);
			DeclareSlider(
				control_bloomscale,
				LOCTEXT("control_bloomscale", "Bloom Scale"),//�ع�����ֵ
				0.f, 64.f,
				InOutPPSettings.BloomSizeScale
			);
		}

		if (bEffectUIAllowed[ChromaticAberration] &&
			InOutPPSettings.SceneFringeIntensity > 0.f)
		{
			DeclareSlider(
				control_scenefringeintensity,
				LOCTEXT("control_chromaticaberration", "Chromatic Aberration"),//ɫ��
				0.f, 15.f, // note: FPostProcesssSettings metadata says range is 0./5. but larger values have been seen in the wild 
				InOutPPSettings.SceneFringeIntensity
			);
		}

		bUIControlsNeedRebuild = false;
	}

	// postprocessing is based upon postprocessing settings at session start time (avoids set of
	// UI tweakables changing due to the camera wandering between postprocessing volumes, also
	// avoids most discontinuities where stereo and panoramic captures can also wander between
	// postprocessing volumes during the capture process)
	// ������ڻỰ��ʼʱ��ĺ������ã���������������ں����֮���ǻ������µ� UI �ɵ����������仯��
	// �������˴�����������ԣ�������������ȫ������Ҳ�����ڲ���������ں����֮���ǻ���
	InOutPPSettings = UEPostProcessingOriginal;

	// update values where corresponding controls are in use
	//��������ʹ����Ӧ�ؼ���ֵ
	if (ProcessUISlider(control_dofscale, InOutPPSettings.DepthOfFieldScale))
	{
		InOutPPSettings.bOverride_DepthOfFieldScale = 1;
	}
	if (ProcessUISlider(control_doffocalregion, InOutPPSettings.DepthOfFieldFocalRegion))
	{
		InOutPPSettings.bOverride_DepthOfFieldFocalRegion = 1;
	}
	if (ProcessUISlider(control_dofsensorwidth, InOutPPSettings.DepthOfFieldSensorWidth))
	{
		InOutPPSettings.bOverride_DepthOfFieldSensorWidth = 1;
	}
	if (ProcessUISlider(control_doffocaldistance, InOutPPSettings.DepthOfFieldFocalDistance))
	{
		InOutPPSettings.bOverride_DepthOfFieldFocalDistance = 1;
	}
	if (ProcessUISlider(control_dofdepthbluramount, InOutPPSettings.DepthOfFieldDepthBlurAmount))
	{
		InOutPPSettings.bOverride_DepthOfFieldDepthBlurAmount = 1;
	}
	if (ProcessUISlider(control_dofdepthblurradius, InOutPPSettings.DepthOfFieldDepthBlurRadius))
	{
		InOutPPSettings.bOverride_DepthOfFieldDepthBlurRadius = 1;
	}
	if (ProcessUISlider(control_bloomintensity, InOutPPSettings.BloomIntensity))
	{
		InOutPPSettings.bOverride_BloomIntensity = 1;
	}
	if (ProcessUISlider(control_bloomscale, InOutPPSettings.BloomSizeScale))
	{
		InOutPPSettings.bOverride_BloomSizeScale = 1;
	}
	if (ProcessUISlider(control_scenefringeintensity, InOutPPSettings.SceneFringeIntensity))
	{
		InOutPPSettings.bOverride_SceneFringeIntensity = 1;
	}
}

//�������������Ӵ��������������
//�����ÿһ֡���ڸɵ���
bool FNVAnselCameraPhotographyPrivate::UpdateCamera(FMinimalViewInfo& InOutPOV, APlayerCameraManager* PCMgr)
{
	check(PCMgr != nullptr);
	
	bool bGameCameraCutThisFrame = false;
	//�Ƿ�ǿ�ƽ�ֹ
	bForceDisallow = false; 
	if (!bAnselSessionActive)
	{
		//��ȡ���洢һЩӰ�� Ansel �Ự���õ���ͼ��ϸ��Ϣ��
		//���� Ansel �ص�����һ������Ⱦ����Ϸ�߳��ϣ�������Щ��ͼ��ϸ��Ϣ���ܲ���ȫ
		bIsOrthoProjection = (InOutPOV.ProjectionMode == ECameraProjectionMode::Orthographic);
		//��ͼ�Ŀͻ��� ָ�� = ������� ��ȡ���硪����ȡ���Ӵ�
		//�����ֹ�Ƿ���״̬�¹���
		if (UGameViewportClient* ViewportClient = PCMgr->GetWorld()->GetGameViewport())
		{
			// ǿ�ƽ�ֹ = ǿ�ƾ�ֹ���� �Ӵ��ͻ���- ��ȡ��ǰ���������� �������ڣ� �������Ͳ�Ϊ��
			bForceDisallow = bForceDisallow || (ViewportClient->GetCurrentSplitscreenConfiguration() != ESplitScreenType::None); // forbid if in splitscreen.
		}

		// ��ֹͼ��ץȡ ����� ����ģʽ����VRģʽ
		bForceDisallow = bForceDisallow || (GEngine->IsStereoscopic3D());

		// ���ϼ��ĳЩ��Ϸ�����в��������ĵ����ݣ���ܷ��� Ҫ����ȫ���³�ʼ�� Ansel
		// 1.ɾ�� ���絽�׵�����

		// �������ȡ�����ָ�� ���Ҳ�Ϊ�գ�û��
		if (const UWorld* World = GEngine->GetWorld())
		{
			
			//�õ���������  �������� = ���� ��ȡ���� ��ȡ��������
			const AWorldSettings* WorldSettings = GEngine->GetWorld()->GetWorldSettings();
			//�������� ���� �������õ����� ������0 
			if (WorldSettings && WorldSettings->WorldToMeters != 0.f)
			{
				//������������� = �������� ��������
				RequiredWorldToMeters = WorldSettings->WorldToMeters;
			}
		}
		
		// ��� FOV Լ������ - ���ڶಿ�ֿ�����Ƭ������Ҫ	
		//��ҿ��� PC = ���������� �õ� ��ҵĿ����� �����������Ϊ��
		if (const APlayerController* PC = PCMgr->GetOwningPlayerController())
		{
			// ��� ������� = ��ҿ����� ��ȡ�ı�����Ҳ�Ϊ��
			if (const ULocalPlayer* LocalPlayer = PC->GetLocalPlayer())
			{
				// ��Ϸ�Ӵ��Ĵ���  �Ӵ����� = ���ص���� ���Ӵ�����  ��Ϊ��
				if (const UGameViewportClient* ViewportClient = LocalPlayer->ViewportClient)
				{
					// �Ӵ� = �Ӵ����� ��ȡ���Ӵ�
					if (const FViewport* Viewport = ViewportClient->Viewport)
					{
						// ��ά���� ��ͼ���� = ���� �ߴ�
						const FVector2D& LPViewScale = LocalPlayer->Size;
						// �ߴ� X =
						// �ߴ� Y = 
						uint32 SizeX = FMath::TruncToInt(LPViewScale.X * Viewport->GetSizeXY().X);
						uint32 SizeY = FMath::TruncToInt(LPViewScale.Y * Viewport->GetSizeXY().Y);

						// �ݺ����Լ���ġ�ö�� = ������� - �ݺ����Լ��
						EAspectRatioAxisConstraint AspectRatioAxisConstraint = LocalPlayer->AspectRatioAxisConstraint;

						// (logic from FMinimalViewInfo::CalculateProjectionMatrixGivenView() -) if x is bigger, and we're respecting x or major axis, AND mobile isn't forcing us to be Y axis aligned
						// ������ߴ�X>�ߴ�Y�� ���� ������� == ����� �����ϵ�FOV�� ���� ���ݺ����Լ�� == ����� ������XFOV�� ���� �����ģʽ == ö�پ���ģʽ������ģʽ��
						if (((SizeX > SizeY) && (AspectRatioAxisConstraint == AspectRatio_MajorAxisFOV)) || (AspectRatioAxisConstraint == AspectRatio_MaintainXFOV) || (InOutPOV.ProjectionMode == ECameraProjectionMode::Orthographic))
						{
							//����FOV���� = ˮƽFOV
							RequiredFovType = ansel::kHorizontalFov;
						}
						else
						{
							//ˮƽFOV
							RequiredFovType = ansel::kVerticalFov;
						}
					}
				}
			}
		}
		// ��� ����ǰ����������� ������ ����������� ���� ��ǰ�����FOV�����˱仯��ʱ��
		if (CurrentlyConfiguredWorldToMeters != RequiredWorldToMeters ||
			CurrentlyConfiguredFovType != RequiredFovType)
		{
			//��Ҫ��������Ansel
			ReconfigureAnsel();
		}
	}
	if (bAnselSessionActive)
	{
		APlayerController* PCOwner = PCMgr->GetOwningPlayerController();
		check(PCOwner != nullptr);
		++NumFramesSinceSessionStart;
		if (bTriggerNextTick)
		{
			NumFramesSinceSessionStart = 0;		
			PCOwner->GetWorldSettings()->SetTimeDilation(0.f);
			bTriggerNextCapture = true;		
			bTriggerNextTick = false;	
		}

		if (bAnselCaptureNewlyActive)									
		{ 
			PCMgr->OnPhotographyMultiPartCaptureStart();
			bGameCameraCutThisFrame = true;
			bAnselCaptureNewlyActive = false;
		}
		
		if (bAnselCaptureNewlyFinished)								
		{
			bGameCameraCutThisFrame = true;
			bAnselCaptureNewlyFinished = false;
			PCMgr->OnPhotographyMultiPartCaptureEnd();

			bTriggerNextTick = true;
			PCOwner->GetWorldSettings()->SetTimeDilation(fTimeDilationBeforeSession);
			PCOwner->SetPause(false);
			bPausedInternally = false;
		}					
		if (bAnselSessionWantDeactivate)					
		{
			bAnselSessionActive = false;
			bAnselSessionWantDeactivate = false;
			if (bAutoPostprocess)
			{
				if (bWasShowingHUDBeforeSession)
				{
					PCOwner->MyHUD->ShowHUD();
				}
				if (bWereSubtitlesEnabledBeforeSession)
				{
					UGameplayStatics::SetSubtitlesEnabled(true);
				}
				if (bWasFadingEnabledBeforeSession)
				{
					PCMgr->bEnableFading = true;
				}
			}

			GAreScreenMessagesEnabled = bWasScreenMessagesEnabledBeforeSession;
			if (bAutoPause && !bWasPausedBeforeSession)
			{
				PCOwner->GetWorldSettings()->SetTimeDilation(fTimeDilationBeforeSession);   
				PCOwner->SetPause(false);
				bPausedInternally = false;
			}

			PCMgr->GetWorld()->bIsCameraMoveableWhenPaused = bWasMovableCameraBeforeSession;

			TSharedPtr<GenericApplication> PlatformApplication = FSlateApplicationBase::Get().GetPlatformApplication();
			if (PlatformApplication.IsValid() && PlatformApplication->Cursor.IsValid())
			{
				PlatformApplication->Cursor->Show(true); 
			}

			for (auto &foo : InitialCVarMap)
			{
				if (foo.Value.cvar)
					foo.Value.cvar->SetWithCurrentPriority(foo.Value.fInitialVal);
			}
			InitialCVarMap.Empty(); 
			bHighQualityModeIsSetup = false;
			PCMgr->OnPhotographySessionEnd();
		}
		else
		{
			bCameraIsInOriginalState = false;	
			//����������
			if (bAnselSessionNewlyActive)					
			{
				NumFramesSinceSessionStart = 0;
				PCMgr->OnPhotographySessionStart();

				bAutoPause = !!CVarPhotographyAutoPause->GetInt();
				bAutoPostprocess = !!CVarPhotographyAutoPostprocess->GetInt();
				bRayTracingEnabled = IsRayTracingEnabled();

				bWasPausedBeforeSession = PCOwner->IsPaused();
				bWasMovableCameraBeforeSession = PCMgr->GetWorld()->bIsCameraMoveableWhenPaused;
				PCMgr->GetWorld()->bIsCameraMoveableWhenPaused = true;
				if (bAutoPause && !bWasPausedBeforeSession)
				{
					fTimeDilationBeforeSession = PCOwner->GetWorldSettings()->GetEffectiveTimeDilation();
					PCOwner->GetWorldSettings()->SetTimeDilation(0.f); 
				}
				SetUpSessionCVars();
				bWasScreenMessagesEnabledBeforeSession = GAreScreenMessagesEnabled;
				GAreScreenMessagesEnabled = false;
				bWasFadingEnabledBeforeSession = PCMgr->bEnableFading;
				bWasShowingHUDBeforeSession = PCOwner->MyHUD && PCOwner->MyHUD->bShowHUD;
				bWereSubtitlesEnabledBeforeSession = UGameplayStatics::AreSubtitlesEnabled();
				if (bAutoPostprocess)
				{
					if (bWasShowingHUDBeforeSession)
					{
						PCOwner->MyHUD->ShowHUD(); // toggle off
					}
					UGameplayStatics::SetSubtitlesEnabled(false);
					PCMgr->bEnableFading = false;
				}
				bUIControlsNeedRebuild = true;

				UECameraPrevious = InOutPOV;
				UECameraOriginal = InOutPOV;
				FMinimalViewToAnselCamera(AnselCamera, InOutPOV);
				ansel::updateCamera(AnselCamera);
				AnselCameraOriginal = AnselCamera;
				AnselCameraPrevious = AnselCamera;

				bCameraIsInOriginalState = true;
				bAnselSessionNewlyActive = false;
				//��Ӻ������ã��������Ա���Sequnce͵��
				AActor* AnselCapture = UGameplayStatics::GetActorOfClass(GEngine->GetCurrentPlayWorld(), AAnselCapture::StaticClass());
				if (AnselCapture != NULL)
				{
					FString ActorName = AnselCapture->GetName();
					UE_LOG(LogAnsel, Log, TEXT("Photography can find world!Name:%s"), *ActorName);
					AAnselCapture* AnselCaptureMonster = static_cast<AAnselCapture*>(AnselCapture);
					AnselCaptureMonster->CallSequncePlayer();
				}
			}
			else
			{
				UECameraPrevious = InOutPOV;
				UECameraOriginal = InOutPOV;
				FMinimalViewToAnselCamera(AnselCamera, InOutPOV);
				ansel::updateCamera(AnselCamera);

				AnselCameraOriginal = AnselCamera;
				AnselCameraPrevious = AnselCamera;
				if (!bAnselCaptureActive)
				{
					bCameraIsInOriginalState = BlueprintModifyCamera(AnselCamera, PCMgr); 
				}
			}
			if (NumFramesSinceSessionStart == 2)		
			{
				if (bAutoPause && !bWasPausedBeforeSession)
				{
					PCOwner->SetPause(true);
					bPausedInternally = true;
				}
			}
			AnselCameraToFMinimalView(InOutPOV, AnselCamera);
			AnselCameraPrevious = AnselCamera;
		}
		if (bAnselCaptureActive)
		{
			InOutPOV.bConstrainAspectRatio = false;
		}

	}
	
	if (bTriggerNextCapture)
	{
		INPUT SpaceBar = { 0 };
		SpaceBar.type = INPUT_KEYBOARD;
		SpaceBar.ki.wVk = VK_SPACE;
		SendInput(1, &SpaceBar, sizeof(INPUT));
		SpaceBar.ki.dwFlags = KEYEVENTF_KEYUP;
		SendInput(1, &SpaceBar, sizeof(INPUT));
		bTriggerNextCapture = false;
	}
	
	return bGameCameraCutThisFrame;

}


//����ץȡ��ֵ�Ķ���(C���ֵ������ û�����õ�ֵ ���Աȣ����ã�ʹ�����ȼ�)
void FNVAnselCameraPhotographyPrivate::SetCapturedCVarPredicated(const char* CVarName, float valueIfNotReset, std::function<bool(const float, const float)> comparison, bool wantReset, bool useExistingPriority)
{
	//����ֵ̨��ָ��
	CVarInfo* info = nullptr;
	// ��ʼ��Cֵ��ӳ�䡣������Cֵ����|| ץȡCֵ��Cֵ����
	if (InitialCVarMap.Contains(CVarName) || CaptureCVar(CVarName))
	{
		//��Ϣ = ��ʼ��CֵMap
		info = &InitialCVarMap[CVarName];
		//��� ����Ϣ-Cֵ �Ա� �Աȣ����ֵû�����ã� ��Ϣ - ��ʼ����ֵ��
		if (info->cvar && comparison(valueIfNotReset, info->fInitialVal))
		{
			//ʹ���Ѿ����ڵ����ȼ�
			if (useExistingPriority)
				//��Ϣ ֵ - ���� ��ǰ���ȼ�����Ҫ������ ����Ϣ�õ��ĳ�ʼ����
				info->cvar->SetWithCurrentPriority(wantReset ? info->fInitialVal : valueIfNotReset);
			else
				//��Ϣ ֵ ���ã���Ҫ���� �� ��Ϣ ��ʼ����ֵ �� û�����õ�ֵ��
				info->cvar->Set(wantReset ? info->fInitialVal : valueIfNotReset);
		}
	}
	if (!(info && info->cvar)) UE_LOG(LogAnsel, Log, TEXT("CVar used by Ansel not found: %s"), CVarName);
}

//����ץȡ��Cֵ������ Cֵ������ ��û�����õ�ֵ ���Ƿ���Ҫ���ã��Ƿ�ʹ�ô��ڵ����ȼ���
void FNVAnselCameraPhotographyPrivate::SetCapturedCVar(const char* CVarName, float valueIfNotReset, bool wantReset, bool useExistingPriority)
{
	//�������һ���£���һЩ����ʡȥһЩ��ʡ���Ա�ֵ��
	SetCapturedCVarPredicated(CVarName, valueIfNotReset,
		[](float, float) { return true; },
		wantReset, useExistingPriority);
}


//������Ⱦ����ΪͼƬץȡ���������ã�
void FNVAnselCameraPhotographyPrivate::ConfigureRenderingSettingsForPhotography(FPostProcessSettings& InOutPostProcessingSettings)
{
//����һ�κ궨��	
// ����_Cֵ�����֣�ֵ��  
#define QUALITY_CVAR(NAME,BOOSTVAL) SetCapturedCVar(NAME, BOOSTVAL, !bHighQualityModeDesired, true)
// ���������ֵ
#define QUALITY_CVAR_AT_LEAST(NAME,BOOSTVAL) SetCapturedCVarPredicated(NAME, BOOSTVAL, std::greater<float>(), !bHighQualityModeDesired, true)
// ���������ֵ
#define QUALITY_CVAR_AT_MOST(NAME,BOOSTVAL) SetCapturedCVarPredicated(NAME, BOOSTVAL, std::less<float>(), !bHighQualityModeDesired, true)
// ����������ȼ���ֵ
#define QUALITY_CVAR_LOWPRIORITY_AT_LEAST(NAME,BOOSTVAL) SetCapturedCVarPredicated(NAME, BOOSTVAL, std::greater<float>(), !bHighQualityModeDesired, false)
	
	// ���Cֵ�����������.��ȡ�����߳��ϵ�ֵ
	if (CVarAllowHighQuality.GetValueOnAnyThread()
		//���� ������ģʽ���� ��= ������ģʽ
		&& bHighQualityModeIsSetup != bHighQualityModeDesired
		// (���ڲ���ͣ���� ���Զ���ͣ)
		&& (bPausedInternally || !bAutoPause) // <- don't start overriding vars until truly paused
		)
	{
		// Pump up (or reset) the quality. 

		// bring rendering up to (at least) 100% resolution, but won't override manually set value on console
		// ��ͼ���������
		QUALITY_CVAR_LOWPRIORITY_AT_LEAST("r.ScreenPercentage", 100);

		// most of these are similar to typical cinematic sg.* scalability settings, toned down a little for performance

		// can be a mild help with reflections quality
		// ��������
		QUALITY_CVAR("r.gbufferformat", 5); // 5 = highest precision

		// bias various geometry LODs 
		// ģ��LOD����
		QUALITY_CVAR_AT_MOST("r.staticmeshloddistancescale", 0.25f); // large quality bias
		QUALITY_CVAR_AT_MOST("r.landscapelodbias", -2);
		QUALITY_CVAR_AT_MOST("r.skeletalmeshlodbias", -4); // big bias here since when paused this never gets re-evaluated and the camera could roam to look at a skeletal mesh far away

		// ~sg.AntiAliasingQuality @ cine 
		// ���������
		QUALITY_CVAR("r.postprocessaaquality", 6); // 6 == max
		QUALITY_CVAR_AT_LEAST("r.defaultfeature.antialiasing", 2); // TAA or higher
		QUALITY_CVAR_AT_LEAST("r.ngx.dlss.quality", 2); // high-quality mode for DLSS if in use

		// ~sg.EffectsQuality @ cinematic
		// Ч������
		QUALITY_CVAR_AT_LEAST("r.TranslucencyLightingVolumeDim", 64);
		QUALITY_CVAR("r.RefractionQuality", 2);
		QUALITY_CVAR("r.SSR.Quality", 4);
		// QUALITY_CVAR("r.SceneColorFormat", 4); // no - don't really want to mess with this
		QUALITY_CVAR("r.TranslucencyVolumeBlur", 1);
		//��������
		QUALITY_CVAR("r.MaterialQualityLevel", 1); // 0==low, -> 1==high <- , 2==medium
		QUALITY_CVAR("r.SSS.Scale", 1);
		QUALITY_CVAR("r.SSS.SampleSet", 2);
		QUALITY_CVAR("r.SSS.Quality", 1);
		QUALITY_CVAR("r.SSS.HalfRes", 0);
		//QUALITY_CVAR_AT_LEAST("r.EmitterSpawnRateScale", 1.f); // no - not sure this has a point when game is paused
		QUALITY_CVAR("r.ParticleLightQuality", 2);
		QUALITY_CVAR("r.DetailMode", 2);

		// ~sg.PostProcessQuality @ cinematic
		// ��������
		QUALITY_CVAR("r.AmbientOcclusionMipLevelFactor", 0.4f);
		QUALITY_CVAR("r.AmbientOcclusionMaxQuality", 100);
		QUALITY_CVAR("r.AmbientOcclusionLevels", -1);
		QUALITY_CVAR("r.AmbientOcclusionRadiusScale", 1.f);
		QUALITY_CVAR("r.DepthOfFieldQuality", 4);
		QUALITY_CVAR_AT_LEAST("r.RenderTargetPoolMin", 500); // ?
		QUALITY_CVAR("r.LensFlareQuality", 3);
		QUALITY_CVAR("r.SceneColorFringeQuality", 1);
		QUALITY_CVAR("r.BloomQuality", 5);
		QUALITY_CVAR("r.FastBlurThreshold", 100);
		QUALITY_CVAR("r.Upscale.Quality", 3);
		QUALITY_CVAR("r.Tonemapper.GrainQuantization", 1);
		QUALITY_CVAR("r.LightShaftQuality", 1);
		QUALITY_CVAR("r.Filter.SizeScale", 1);
		QUALITY_CVAR("r.Tonemapper.Quality", 5);
		QUALITY_CVAR("r.DOF.Gather.AccumulatorQuality", 1);
		QUALITY_CVAR("r.DOF.Gather.PostfilterMethod", 1);
		QUALITY_CVAR("r.DOF.Gather.EnableBokehSettings", 1);
		QUALITY_CVAR_AT_LEAST("r.DOF.Gather.RingCount", 5);
		QUALITY_CVAR("r.DOF.Scatter.ForegroundCompositing", 1);
		QUALITY_CVAR("r.DOF.Scatter.BackgroundCompositing", 2);
		QUALITY_CVAR("r.DOF.Scatter.EnableBokehSettings", 1);
		QUALITY_CVAR("r.DOF.Scatter.MaxSpriteRatio", 0.1f);
		QUALITY_CVAR("r.DOF.Recombine.Quality", 2);
		QUALITY_CVAR("r.DOF.Recombine.EnableBokehSettings", 1);
		QUALITY_CVAR("r.DOF.TemporalAAQuality", 1);
		QUALITY_CVAR("r.DOF.Kernel.MaxForegroundRadius", 0.025f);
		QUALITY_CVAR("r.DOF.Kernel.MaxBackgroundRadius", 0.025f);

		// ~sg.TextureQuality @ cinematic
		// ��ͼ���� @Ӱ�Ӽ�
		QUALITY_CVAR("r.Streaming.MipBias", 0);
		QUALITY_CVAR_AT_LEAST("r.MaxAnisotropy", 16);
		QUALITY_CVAR("r.Streaming.MaxEffectiveScreenSize", 0);
		// intentionally don't mess with streaming pool size, see 'CVarExtreme' section below

		// ~sg.FoliageQuality @ cinematic 
		// ֲ������
		QUALITY_CVAR_AT_LEAST("foliage.DensityScale", 1.f);
		QUALITY_CVAR_AT_LEAST("grass.DensityScale", 1.f);
		// boosted foliage LOD (use distance scale not lod bias - latter is buggy)
		// Lod �������Ÿ�Ϊ֮ǰ���ı�
		QUALITY_CVAR_AT_LEAST("foliage.LODDistanceScale", 4.f);

		// ~sg.ViewDistanceQuality @ cine but only mild draw distance boost
		QUALITY_CVAR_AT_LEAST("r.viewdistancescale", 2.0f); // or even more...?

		// ~sg.ShadowQuality @ cinematic //�ƹ���Ӱ����
		QUALITY_CVAR_AT_LEAST("r.LightFunctionQuality", 2);
		QUALITY_CVAR("r.ShadowQuality", 5);
		QUALITY_CVAR_AT_LEAST("r.Shadow.CSM.MaxCascades", 10);
		QUALITY_CVAR_AT_LEAST("r.Shadow.MaxResolution", 4096);
		QUALITY_CVAR_AT_LEAST("r.Shadow.MaxCSMResolution", 4096);
		QUALITY_CVAR_AT_MOST("r.Shadow.RadiusThreshold", 0.f);
		QUALITY_CVAR("r.Shadow.DistanceScale", 1.f);
		QUALITY_CVAR("r.Shadow.CSM.TransitionScale", 1.f);
		QUALITY_CVAR("r.Shadow.PreShadowResolutionFactor", 1.f);
		QUALITY_CVAR("r.AOQuality", 2);
		QUALITY_CVAR("r.VolumetricFog", 1);
		QUALITY_CVAR("r.VolumetricFog.GridPixelSize", 4);
		QUALITY_CVAR("r.VolumetricFog.GridSizeZ", 128);
		QUALITY_CVAR_AT_LEAST("r.VolumetricFog.HistoryMissSupersampleCount", 16);
		QUALITY_CVAR_AT_LEAST("r.LightMaxDrawDistanceScale", 4.f);

		// pump up the quality of raytracing features, though we won't necessarily turn them on if the game doesn't already have them enabled
		// �����׷����� ������������Ų���
		if (bRayTracingEnabled)
		{
			QUALITY_CVAR_AT_LEAST("D3D12.PSO.StallTimeoutInMs", 8000.0f); // the high-quality RTPSOs may have to be built from scratch the first time; temporarily raise this limit in the hope of avoiding the rare barf-outs.

			/*** HIGH-QUALITY MODE DOES *NOT* FORCE GI ON ***/
			// Don't tweak GI parameters right now - its performance is super-sensitive to changes and a long long frame will cause a device-disconnect.
			//QUALITY_CVAR_AT_MOST("r.RayTracing.GlobalIllumination.DiffuseThreshold", 0); // artifact avoidance

			/*** HIGH-QUALITY MODE DOES *NOT* FORCE RT AO ON ***/
			QUALITY_CVAR_AT_LEAST("r.RayTracing.AmbientOcclusion.SamplesPerPixel", 3);

			/*** HIGH-QUALITY MODE DOES *NOT* FORCE RT REFLECTIONS ON ***/
			QUALITY_CVAR("r.raytracing.reflections.rendertilesize", 128); // somewhat protect against long frames (from pumped-up quality) causing a device-disconnect
			QUALITY_CVAR_AT_LEAST("r.RayTracing.Reflections.MaxBounces", 2); // ~sweet-spot
			QUALITY_CVAR_AT_LEAST("r.RayTracing.Reflections.MaxRoughness", 0.9f); // speed hit
			QUALITY_CVAR_AT_LEAST("r.RayTracing.Reflections.MaxRayDistance", 1000000.f);
			QUALITY_CVAR("r.RayTracing.Reflections.SortMaterials", 1); // usually some kind of perf win, especially w/above reflection quality
			QUALITY_CVAR("r.RayTracing.Reflections.DirectLighting", 1);
			//QUALITY_CVAR("r.RayTracing.Reflections.EmissiveAndIndirectLighting", 1);// curiously problematic to force, leave alone
			QUALITY_CVAR_AT_LEAST("r.RayTracing.Reflections.Shadows", 1); // -1==auto, 0==off, 1==hard, 2==soft/area(requires high spp)
			QUALITY_CVAR("r.RayTracing.Reflections.HeightFog", 1);
			QUALITY_CVAR("r.RayTracing.Reflections.ReflectionCaptures", 1);
			//QUALITY_CVAR_AT_LEAST("r.RayTracing.Reflections.SamplesPerPixel", 2); // -1==use pp vol // NOPE, don't touch spp right now: 1 is ok, ~10 is good, anywhere in-between is noisy
			QUALITY_CVAR_AT_LEAST("r.RayTracing.Reflections.ScreenPercentage", 100);
			// QUALITY_CVAR("r.RayTracing.Reflections", 1); // FORCE ON: ignore postproc volume flag -- NOPE, there are a couple of RT reflection issues right now which can leave RT reflections much brighter than their raster counterparts
			//QUALITY_CVAR("r.raytracing.reflections.translucency", 1); // hmm, nope, usually good translucency - but the reflection shader appears to apply translucency after roughness-fade so there's some risk of IQ regression here right now.  may enable after more testing.

			/*** HIGH-QUALITY MODE DOES *NOT* FORCE RT TRANSLUCENCY ON ***/
			// ������ģʽ�µ�����
			QUALITY_CVAR_AT_LEAST("r.RayTracing.Translucency.MaxRoughness", 0.9f);
			QUALITY_CVAR_AT_LEAST("r.RayTracing.Translucency.MaxRayDistance", 1000000.f);
			QUALITY_CVAR_AT_LEAST("r.RayTracing.Translucency.MaxRefractionRays", 11); // number of layers of ray penetration, actually regardless of whether refraction is enabled
			QUALITY_CVAR_AT_LEAST("r.RayTracing.Translucency.Shadows", 1); // turn on at least basic quality

			//������ģʽ
			/*** HIGH-QUALITY MODE DOES *NOT* FORCE RT SHADOWS ON ***/
			QUALITY_CVAR("r.RayTracing.Shadow.MaxLights", -1); // unlimited
			QUALITY_CVAR("r.RayTracing.Shadow.MaxDenoisedLights", -1); // unlimited

			// these apply to various RT effects but mostly reflections+translucency
			// Ӧ�ù�׷Ч�� �ķ���+��͸��
			QUALITY_CVAR_AT_LEAST("r.raytracing.lighting.maxshadowlights", 256); // as seen in reflections/translucencies
			QUALITY_CVAR_AT_LEAST("r.RayTracing.lighting.maxlights", 256); // as seen in reflections/translucencies
		}

		 // these are some extreme settings whose quality:risk ratio may be debatable or unproven
		 // ��Щ��һЩ�������ã������������ձȿ���ֵ����ȶ��δ��֤ʵ
		if (CVarExtreme->GetInt())
		{
			// great idea but not until I've proven that this isn't deadly or extremely slow on lower-spec machines:
			// �����⣬��ֱ����֤�����ڵ͹������ϲ��������Ļ�ǳ����ģ�
			QUALITY_CVAR("r.Streaming.LimitPoolSizeToVRAM", 0); // 0 is aggressive but is it safe? seems safe.
			QUALITY_CVAR_AT_LEAST("r.Streaming.PoolSize", 3000); // cine - perhaps redundant when r.streaming.fullyloadusedtextures

			QUALITY_CVAR("r.streaming.hlodstrategy", 2); // probably use 0 if using r.streaming.fullyloadusedtextures, else 2
			//QUALITY_CVAR("r.streaming.fullyloadusedtextures", 1); // no - LODs oscillate when overcommitted
			QUALITY_CVAR_AT_LEAST("r.viewdistancescale", 10.f); // cinematic - extreme

			//��׷���������¿������²���
			if (bRayTracingEnabled)
			{
				// higher-IQ thresholds
				QUALITY_CVAR_AT_LEAST("r.RayTracing.Translucency.MaxRoughness", 1.f); // speed hit
				QUALITY_CVAR_AT_LEAST("r.RayTracing.Reflections.MaxRoughness", 1.f); // speed hit

				//QUALITY_CVAR("r.ambientocclusionstaticfraction", 0.f); // trust RT AO/GI...? - needs more testing, doesn't seem a big win

				/*** EXTREME-QUALITY MODE FORCES GI ON ***/
				// first, some IQ:speed tweaks to make GI speed practical
				QUALITY_CVAR("r.raytracing.GlobalIllumination.rendertilesize", 128); // somewhat protect against long frames (from pumped-up quality) causing a device-disconnect
				QUALITY_CVAR("r.RayTracing.GlobalIllumination.ScreenPercentage", 50); // 50% = this is actually a quality DROP by default but it makes the GI speed practical -- requires >>>=2spp though
				QUALITY_CVAR("r.RayTracing.GlobalIllumination.MaxRayDistance", 7500); // ditto; most of the IQ benefit, but often faster than default huge ray distance
				QUALITY_CVAR_AT_LEAST("r.RayTracing.GlobalIllumination.SamplesPerPixel", 4); // at LEAST 2spp needed to reduce significant noise in some scenes, even up to 8+ helps
				QUALITY_CVAR_AT_LEAST("r.RayTracing.GlobalIllumination.NextEventEstimationSamples", 16); // 2==default; 16 necessary for low-light conditions when using only 4spp, else get blotches.  raising estimation samples cheaper than raising spp.
				QUALITY_CVAR_AT_LEAST("r.GlobalIllumination.Denoiser.ReconstructionSamples", 56/*=max*/); // better if only using 4spp @ quarter rez.  default is 16.
				//QUALITY_CVAR_AT_LEAST("r.RayTracing.GlobalIllumination.MaxBounces", 3); // 2+ is sometimes slightly noticable, sloww
				////QUALITY_CVAR("r.RayTracing.GlobalIllumination.EvalSkyLight", 1); // EXPERIMENTAL
				QUALITY_CVAR("r.RayTracing.GlobalIllumination", 1); // FORCE ON: should be fast enough to not TDR(!) with screenpercentage=50... usually a fair IQ win with random content... hidden behind 'EXTREME' mode until I've exercised it more.

				// just not hugely tested:
				QUALITY_CVAR_AT_LEAST("r.RayTracing.StochasticRectLight.SamplesPerPixel", 4);
				//QUALITY_CVAR("r.RayTracing.StochasticRectLight", 1); // 1==suspicious, probably broken
				QUALITY_CVAR_AT_LEAST("r.RayTracing.SkyLight.SamplesPerPixel", 4); // default==-1 UNPROVEN TRY ME
			}

			// just not hugely tested:
			//ֻ��û�о����������ԣ�
			QUALITY_CVAR("r.particlelodbias", -2);

			// unproven or possibly buggy
			//QUALITY_CVAR("r.streaming.useallmips", 1); // removes relative prioritization spec'd by app... unproven that this is a good idea
			//QUALITY_CVAR_AT_LEAST("r.streaming.boost", 9999); // 0 = supposedly use all available vram, but it looks like 0 = buggy
		}

#undef QUALITY_CVAR
#undef QUALITY_CVAR_AT_LEAST
#undef QUALITY_CVAR_AT_MOST
#undef QUALITY_CVAR_LOWPRIORITY_AT_LEAST

		UE_LOG(LogAnsel, Log, TEXT("Photography HQ mode actualized (enabled=%d)"), (int)bHighQualityModeDesired);
		bHighQualityModeIsSetup = bHighQualityModeDesired;
		
	}
	//���Anselץȡ����
	if (bAnselCaptureActive)
	{
		// camera doesn't linger in one place very long so maximize streaming rate
		SetCapturedCVar("r.disablelodfade", 1);
		SetCapturedCVar("r.streaming.framesforfullupdate", 1); // recalc required LODs ASAP
		SetCapturedCVar("r.Streaming.MaxNumTexturesToStreamPerFrame", 0); // no limit
		SetCapturedCVar("r.streaming.numstaticcomponentsprocessedperframe", 0); // 0 = load all pending static geom now
		//��������Զ�����
		if (bAutoPostprocess)
		{
			// force-disable the standard postprocessing effects which are known to
			// be problematic in multi-part shots

			// nerf remaining motion blur �˶�ģ������
			InOutPostProcessingSettings.bOverride_MotionBlurAmount = 1;
			InOutPostProcessingSettings.MotionBlurAmount = 0.f;

			// these effects tile poorly   ��ЩЧ��ƽ��Ч����
			InOutPostProcessingSettings.bOverride_BloomDirtMaskIntensity = 1;
			InOutPostProcessingSettings.BloomDirtMaskIntensity = 0.f;
			InOutPostProcessingSettings.bOverride_LensFlareIntensity = 1;
			InOutPostProcessingSettings.LensFlareIntensity = 0.f;
			InOutPostProcessingSettings.bOverride_VignetteIntensity = 1;
			InOutPostProcessingSettings.VignetteIntensity = 0.f;
			InOutPostProcessingSettings.bOverride_SceneFringeIntensity = 1;
			InOutPostProcessingSettings.SceneFringeIntensity = 0.f;

			// freeze auto-exposure adaptation 
			// �����Զ��ع�����
			InOutPostProcessingSettings.bOverride_AutoExposureSpeedDown = 1;
			InOutPostProcessingSettings.AutoExposureSpeedDown = 0.f;
			InOutPostProcessingSettings.bOverride_AutoExposureSpeedUp = 1;
			InOutPostProcessingSettings.AutoExposureSpeedUp = 0.f;

			// bring rendering up to (at least) full resolution
			// ������Ⱦ��������ȫ�ֱ���
			if (InOutPostProcessingSettings.ScreenPercentage_DEPRECATED < 100.f)
			{
				// note: won't override r.screenpercentage set from console, that takes precedence
				InOutPostProcessingSettings.bOverride_ScreenPercentage_DEPRECATED = 1;
				InOutPostProcessingSettings.ScreenPercentage_DEPRECATED = 100.f;
			}
			// Anselץȡ���� = ץȡ��Ϣ.ץȡ���� ���� ���߷ֱ���ץȡ
			bool bAnselSuperresCaptureActive = AnselCaptureInfo.captureType == ansel::kCaptureTypeSuperResolution;
			// ���ץȡ������ 360���� ���� ������ģʽ
			bool bAnselStereoCaptureActive = AnselCaptureInfo.captureType == ansel::kCaptureType360Stereo || AnselCaptureInfo.captureType == ansel::kCaptureTypeStereo;
			if (bAnselStereoCaptureActive)
			{
				// Attempt to nerf DoF in stereoscopic shots where it can be quite unpleasant for the viewer
				// ���������徵ͷ��nerf DoF����Թ�����˵���ܷǳ������
				InOutPostProcessingSettings.bOverride_DepthOfFieldScale = 1;
				InOutPostProcessingSettings.DepthOfFieldScale = 0.f; // BokehDOF
				InOutPostProcessingSettings.bOverride_DepthOfFieldNearBlurSize = 1;
				InOutPostProcessingSettings.DepthOfFieldNearBlurSize = 0.f; // GaussianDOF
				InOutPostProcessingSettings.bOverride_DepthOfFieldFarBlurSize = 1;
				InOutPostProcessingSettings.DepthOfFieldFarBlurSize = 0.f; // GaussianDOF
				InOutPostProcessingSettings.bOverride_DepthOfFieldDepthBlurRadius = 1;
				InOutPostProcessingSettings.DepthOfFieldDepthBlurRadius = 0.f; // CircleDOF
				InOutPostProcessingSettings.bOverride_DepthOfFieldVignetteSize = 1;
				InOutPostProcessingSettings.DepthOfFieldVignetteSize = 200.f; // Scene.h says 200.0 means 'no effect'
			}
			//����ansel����ץȡ����
			if (!bAnselSuperresCaptureActive)
			{
				// Disable SSR in multi-part shots unless taking a super-resolution shot; SSR *usually* degrades gracefully in tiled shots, and super-resolution mode in Ansel has an 'enhance' option which repairs any lingering SSR artifacts quite well.
				InOutPostProcessingSettings.bOverride_ScreenSpaceReflectionIntensity = 1;
				InOutPostProcessingSettings.ScreenSpaceReflectionIntensity = 0.f;
			}
		}
	}
}

//���� �ỰCֵ
void FNVAnselCameraPhotographyPrivate::SetUpSessionCVars()
{
	// This set of CVar tweaks are good - or necessary - for photographic sessions
	// ����CVar����������Ӱ������˵�Ǻõ� - �����Ǳ�Ҫ��
	{
		SetCapturedCVar("r.oneframethreadlag", 1); // ansel needs frame latency to be predictable ansel ��Ҫ֡�ӳٲ��ܿ�Ԥ��

		// these are okay tweaks to streaming heuristics to reduce latency of full texture loads or minimize VRAM waste
		// ��Щ���Ƕ���ʽ���ʽ�ĵ������Լ�������������ص��ӳٻ�����޶ȵؼ���VRAM�˷�
		// �ϸ����ȿ��ǵ�ǰ�ɼ�������
		SetCapturedCVar("r.streaming.minmipforsplitrequest", 1); // strictly prioritize what's visible right now 
		//��ʾ���潵��ģ����������ȼ�...��
		SetCapturedCVar("r.streaming.hiddenprimitivescale", 0.001f); // hint to engine to deprioritize obscured textures...?
		SetCapturedCVar("r.Streaming.Boost", 1);
		//�� nerfs �˶�ģ�����ڷ��ַ�
		SetCapturedCVar("r.motionblurquality", 0); // this nerfs motion blur for non-characters
	}
}

//���º������ã��������� ��������������ã�
void FNVAnselCameraPhotographyPrivate::UpdatePostProcessing(FPostProcessSettings& InOutPostProcessingSettings)
{
	// ansel�Ự����
	if (bAnselSessionActive)
	{
		//�����Զ���UI���ƣ�������������ĺ������ã��Ƿ�����UI���Ƶ��ع���
		DoCustomUIControls(InOutPostProcessingSettings, bUIControlsNeedRebuild);
		//������Ⱦ���ã�������������ĺ������ã�
		ConfigureRenderingSettingsForPhotography(InOutPostProcessingSettings);
	}
}

// ����Ự
void FNVAnselCameraPhotographyPrivate::StartSession()
{
	ansel::startSession();
}

// ֹͣ�Ự
void FNVAnselCameraPhotographyPrivate::StopSession()
{
	ansel::stopSession();
}

//Ĭ��Լ�������(�µ����λ�ã�Ԥ�����λ�ã�ԭ�����λ�ã�������λ�� ������������)
void FNVAnselCameraPhotographyPrivate::DefaultConstrainCamera(const FVector NewCameraLocation, const FVector PreviousCameraLocation, const FVector OriginalCameraLocation, FVector& OutCameraLocation, APlayerCameraManager* PCMgr)
{
	// let proposed camera through unmodified by default
	// �ý�������ͨ��δ�޸�Ĭ�������
	OutCameraLocation = NewCameraLocation;

	// First, constrain by distance ͨ������Լ��
	FVector ConstrainedLocation;
	// ������
	float MaxDistance = CVarConstrainCameraDistance->GetFloat();
	//ͨ������Լ�����
	UAnselFunctionLibrary::ConstrainCameraByDistance(PCMgr, NewCameraLocation, PreviousCameraLocation, OriginalCameraLocation, ConstrainedLocation, MaxDistance);

	// Second, constrain against collision geometry Լ����ײ����ͼ��
	// �ڶ��� ͨ��ģ��Լ�����
	UAnselFunctionLibrary::ConstrainCameraByGeometry(PCMgr, ConstrainedLocation, PreviousCameraLocation, OriginalCameraLocation, OutCameraLocation);
}

// ����Ự��״̬ Ansel����Ự�ص���Ansel�Ự���� ���û�ָ�룩
ansel::StartSessionStatus FNVAnselCameraPhotographyPrivate::AnselStartSessionCallback(ansel::SessionConfiguration& settings, void* userPointer)
{
	//ansel ����Ự״̬   ���Ự״̬��Ϊ  disallow ���������
	ansel::StartSessionStatus AnselSessionStatus = ansel::kDisallowed;
	//˽��ʵ�� = ��̬ת�����û�ָ�룩
	FNVAnselCameraPhotographyPrivate* PrivateImpl = static_cast<FNVAnselCameraPhotographyPrivate*>(userPointer);
	//���ָ��ǿ�
	check(PrivateImpl != nullptr);
	// ʵ�� ����ǿ�ƽ��� && ͼ��ץȡ��ȡ������ && ���Ǳ༭��״̬
	if (!PrivateImpl->bForceDisallow && CVarPhotographyAllow->GetInt() && !GIsEditor)
	{
		//��ͣ���� 
		bool bPauseAllowed = true;
		//���öಿ�� = ����ͼ��ץȡ�ಿ�� - ��ȡ����
		bool bEnableMultipart = !!CVarPhotographyEnableMultipart->GetInt();

		// �����ƶ� 
		settings.isTranslationAllowed = true;
		settings.isFovChangeAllowed = !PrivateImpl->bIsOrthoProjection;   //��������Ͳ�֧���ƶ�
		settings.isRotationAllowed = true;								//�Ƿ�������ת
		settings.isPauseAllowed = bPauseAllowed;						//bPauseAllowed//�Ƿ�������ͣ
		settings.isHighresAllowed = bEnableMultipart;								//bEnableMultipart	//�Ƿ�����߷ֱ���
		settings.is360MonoAllowed = bEnableMultipart;					//�Ƿ�����360ģʽ
		settings.is360StereoAllowed = bEnableMultipart;							//bEnableMultipart //�Ƿ�����360����

		PrivateImpl->bAnselSessionActive = true;						//Ansel �Ự����
		PrivateImpl->bAnselSessionNewlyActive = true;					//Ansel�µĻỰ����
		PrivateImpl->bHighQualityModeDesired = false;					// ���Ǹ�����ģʽ

		AnselSessionStatus = ansel::kAllowed;							//Ansel�ĻỰ״̬��Ϊ����
	}

	UE_LOG(LogAnsel, Log, TEXT("Photography camera session attempt started, Allowed=%d, ForceDisallowed=%d"), int(AnselSessionStatus == ansel::kAllowed), int(PrivateImpl->bForceDisallow));
	
	//Ansel��״̬
	return AnselSessionStatus;
}

void FNVAnselCameraPhotographyPrivate::AnselStopSessionCallback(void* userPointer)
{
	//
	FNVAnselCameraPhotographyPrivate* PrivateImpl = static_cast<FNVAnselCameraPhotographyPrivate*>(userPointer);
	//���ʵ���Ƿ�Ϊ��
	check(PrivateImpl != nullptr);
	//˽��ʵ�� ��Ansel�Ự������&&ʵ�� Ansel�Ự�µļ��
	if (PrivateImpl->bAnselSessionActive && PrivateImpl->bAnselSessionNewlyActive)
	{
		// if we've not acted upon the new session at all yet, then just don't.
		PrivateImpl->bAnselSessionActive = false;
	}
	else
	{
		PrivateImpl->bAnselSessionWantDeactivate = true;
	}

	UE_LOG(LogAnsel, Log, TEXT("Photography camera session end"));
}

//Ansel ��ʼ����Ļص����������� anselץȡ��������Ϣ���û�ָ�룩
void FNVAnselCameraPhotographyPrivate::AnselStartCaptureCallback(const ansel::CaptureConfiguration& CaptureInfo, void* userPointer)
{
	//���Ҷ���ʵ��
	FNVAnselCameraPhotographyPrivate* PrivateImpl = static_cast<FNVAnselCameraPhotographyPrivate*>(userPointer);
	check(PrivateImpl != nullptr);
	//ʵ��ץȡ�����ʶ����
	PrivateImpl->bAnselCaptureActive = true;
	//ʵ����ץȡ�µļ���
	PrivateImpl->bAnselCaptureNewlyActive = true;
	//ʵ����ץȡ��Ϣ = ansel��ץȡ��Ϣ
	PrivateImpl->AnselCaptureInfo = CaptureInfo;

	UE_LOG(LogAnsel, Log, TEXT("Photography camera multi-part capture started"));
}

//����ص��������û�֪����ͼ�Ѿ����
void FNVAnselCameraPhotographyPrivate::AnselStopCaptureCallback(void* userPointer)
{
	FNVAnselCameraPhotographyPrivate* PrivateImpl = static_cast<FNVAnselCameraPhotographyPrivate*>(userPointer);
	check(PrivateImpl != nullptr);
	//ʵ��ץȡ�ر�  = ��
	PrivateImpl->bAnselCaptureActive = false;
	//ʵ��ץȡ �µĹر�Ϊ��
	PrivateImpl->bAnselCaptureNewlyFinished = true;  //@@@@��Ҫ

	UE_LOG(LogAnsel, Log, TEXT("Photography camera multi-part capture end"));
}

//Ansel���������Ļص����Ƿ������ģʽ��һ��Cpp�û���ָ�룩
void FNVAnselCameraPhotographyPrivate::AnselChangeQualityCallback(bool isHighQuality, void* ACPPuserPointer)
{
	FNVAnselCameraPhotographyPrivate* PrivateImpl = static_cast<FNVAnselCameraPhotographyPrivate*>(ACPPuserPointer);
	check(PrivateImpl != nullptr);
	//ʵ��������Ƿ������ģʽ ������ = �Ƿ������ģʽ
	PrivateImpl->bHighQualityModeDesired = isHighQuality;

	UE_LOG(LogAnsel, Log, TEXT("Photography HQ mode toggle (%d)"), (int)isHighQuality);
}

//���Ҳ����Ҫ
void FNVAnselCameraPhotographyPrivate::ReconfigureAnsel()
{
	check(AnselConfig != nullptr);
	//ansel���� - �û�ָ�� = ����
	AnselConfig->userPointer = this;
	//���ֻص�������ָ��
	AnselConfig->startSessionCallback = AnselStartSessionCallback;
	AnselConfig->stopSessionCallback = AnselStopSessionCallback;
	AnselConfig->startCaptureCallback = AnselStartCaptureCallback;
	AnselConfig->stopCaptureCallback = AnselStopCaptureCallback;
	AnselConfig->changeQualityCallback = AnselChangeQualityCallback;

	// Getting fovType wrong can lead to multi-part captures stitching incorrectly, especially 360 shots
	// fovType������ܵ��¶ಿ�ֲ���ƴ�Ӳ���ȷ��������360������
	AnselConfig->fovType = RequiredFovType;
	// ���� - ��Ϸ��ͼ    ����    ������Ϸ��ͼ��ȡ������ֵ   ����  �����ȡ��Ϸ�Ӵ�-��ȡԭ��������ֵ
	if (GEngine->GameViewport && GEngine->GameViewport->GetWindow().IsValid() && GEngine->GameViewport->GetWindow()->GetNativeWindow().IsValid())
	{
		//ansel���õ���Ϸ�Ӵ���� = ������Ϸ�Ӵ��Ĵ����µ�ԭ������ Ȼ���ȡos���ڵľ��
		AnselConfig->gameWindowHandle = GEngine->GameViewport->GetWindow()->GetNativeWindow()->GetOSWindowHandle();
	}
	UE_LOG(LogAnsel, Log, TEXT("gameWindowHandle= %p"), AnselConfig->gameWindowHandle);
	//ansel����  ���� �ƶ��ٶ������絥λ��ÿ��  = �ӿ���̨���� �е��ƶ��ٶ� ��ȡ�ĸ���ֵ
	AnselConfig->translationalSpeedInWorldUnitsPerSecond = CVarPhotographyTranslationSpeed->GetFloat();

	//ansel���� �������絥λ = 1/�������絽��
	AnselConfig->metersInWorldUnit = 1.0f / RequiredWorldToMeters;
	UE_LOG(LogAnsel, Log, TEXT("We reckon %f meters to 1 world unit"), AnselConfig->metersInWorldUnit);
	//��������Ƿ�֧��
	AnselConfig->isCameraOffcenteredProjectionSupported = true;
	//���������
	AnselConfig->captureLatency = 0; // important
	//�����ȶ��ӳ�
	AnselConfig->captureSettleLatency = CVarPhotographySettleFrames->GetInt();


	//ansel ��������״̬ ״̬ = Ansell;;�������ã�*ansel���ã�
	ansel::SetConfigurationStatus status = ansel::setConfiguration(*AnselConfig);
	if (status != ansel::kSetConfigurationSuccess)
	{
		UE_LOG(LogAnsel, Log, TEXT("ReconfigureAnsel setConfiguration returned %ld"), (long int)(status));
	}
	//��ǰ����FOV���� = ����FOV����
	CurrentlyConfiguredFovType = RequiredFovType;
	//��ǰ�������絽�� =�������絽��
	CurrentlyConfiguredWorldToMeters = RequiredWorldToMeters;
}

//���Ansel������
void FNVAnselCameraPhotographyPrivate::DeconfigureAnsel()
{
	//���Ansel���ò�Ϊ��
	check(AnselConfig != nullptr);
	//Ansel���õĸ��ֻص����
	AnselConfig->userPointer = nullptr;
	AnselConfig->startSessionCallback = nullptr;
	AnselConfig->stopSessionCallback = nullptr;
	AnselConfig->startCaptureCallback = nullptr;
	AnselConfig->stopCaptureCallback = nullptr;
	AnselConfig->gameWindowHandle = nullptr;
	// ansel ����������״̬  =  Ansel���������ã�*Ansel���ã�
	ansel::SetConfigurationStatus status = ansel::setConfiguration(*AnselConfig);
	//״̬Ϊ���÷ǳɹ���״̬
	if (status != ansel::kSetConfigurationSuccess)
	{
		UE_LOG(LogAnsel, Log, TEXT("DeconfigureAnsel setConfiguration returned %ld"), (long int)(status));
	}
}


//Ansel��ģ�� �̳��� Ansel�Ľӿ���
class FAnselModule : public IAnselModule
{
public:
	//���ģ��
	virtual void StartupModule() override
	{
		//�������ģ�����
		ICameraPhotographyModule::StartupModule();
		//Ansel��Dll�Ƿ��Ѿ�����
		check(!bAnselDLLLoaded);

		// Late-load Ansel DLL.  DLL name has been worked out by the build scripts as ANSEL_DLL
		FString AnselDLLName;	//AnselDll������

		//Ansel�ķ����ļ���Ŀ¼ = 
		FString AnselBinariesRoot = FPaths::FPaths::Combine(IPluginManager::Get().FindPlugin(TEXT("Ansel"))->GetBaseDir(),TEXT("/Binaries/ThirdParty/"));
		// common preprocessor fudge to convert macro expansion into string
		// ���ڽ�����չת��Ϊ�ַ����ĳ���Ԥ����������
#define STRINGIFY(X) STRINGIFY2(X)
#define STRINGIFY2(X) #X
			AnselDLLName = AnselBinariesRoot + TEXT(STRINGIFY(ANSEL_DLL));
			AnselSDKDLLHandle = FPlatformProcess::GetDllHandle(*(AnselDLLName));
			//anselDll����  = AnselSDK�����Ϊ��
		bAnselDLLLoaded = AnselSDKDLLHandle != 0;
		UE_LOG(LogAnsel, Log, TEXT("Tried to load %s : success=%d"), *AnselDLLName, int(bAnselDLLLoaded));
	}
	//�ر�ģ��
	virtual void ShutdownModule() override
	{		
		if (bAnselDLLLoaded)
		{
			//�ͷž��
			FPlatformProcess::FreeDllHandle(AnselSDKDLLHandle);
			//�ͷ�ָ��
			AnselSDKDLLHandle = 0;
			//��������״̬
			bAnselDLLLoaded = false;
		}
		//�������ģ��ر�
		ICameraPhotographyModule::ShutdownModule();
	}
private:
	// �����������Ľӿ��� ���������������
	virtual TSharedPtr< class ICameraPhotography > CreateCameraPhotography() override
	{
		//����һ������ָ��
		TSharedPtr<ICameraPhotography> Photography = nullptr;

		//ʵ����һ�� ͼ�񲶻�ĵ�ʵ��
		FNVAnselCameraPhotographyPrivate* PhotographyPrivate = new FNVAnselCameraPhotographyPrivate();
		if (PhotographyPrivate->IsSupported())
		{
			//���͵�ǿת
			Photography = TSharedPtr<ICameraPhotography>(PhotographyPrivate);
		}
		else
		{
			delete PhotographyPrivate;
		}
		//�����������Ķ���
		return Photography;
	}
};
//ʵ��ģ��ģ�ģ��������ģ������
IMPLEMENT_MODULE(FAnselModule, Ansel)

#undef LOCTEXT_NAMESPACE