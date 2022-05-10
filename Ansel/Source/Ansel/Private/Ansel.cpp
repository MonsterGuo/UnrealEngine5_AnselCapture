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

#include "AnselCapture.h"
#include "Engine/Engine.h"
#include "EngineUtils.h"
#include "Kismet/GameplayStatics.h"
#include "AnselFunctionLibrary.h"
#include <AnselSDK.h>

DEFINE_LOG_CATEGORY_STATIC(LogAnsel, Log, All);

#define LOCTEXT_NAMESPACE "Photography"

//控制台相关参数
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
	FNVAnselCameraPhotographyPrivate(); //构造函数
	virtual ~FNVAnselCameraPhotographyPrivate() override; //析构函数
	virtual bool UpdateCamera(FMinimalViewInfo& InOutPOV, APlayerCameraManager* PCMgr) override; // 更新相机
	virtual void UpdatePostProcessing(FPostProcessSettings& InOutPostProcessSettings) override;	//更新后期
	virtual void StartSession() override;	//开始会话
	virtual void StopSession() override;	//结束会话
	virtual bool IsSupported() override;	//是否支持
	virtual void SetUIControlVisibility(uint8 UIControlTarget, bool bIsVisible) override;	//设置UI可见性
	//默认包含的相机
	virtual void DefaultConstrainCamera(const FVector NewCameraLocation, const FVector PreviousCameraLocation, const FVector OriginalCameraLocation, FVector& OutCameraLocation, APlayerCameraManager* PCMgr) override;
	//获取提供的名称
	virtual const TCHAR* const GetProviderName() override { return TEXT("NVIDIA Ansel"); };
	
	enum econtrols {
		control_dofscale,  //缩放
		control_dofsensorwidth,	//长宽比
		control_doffocalregion, //重点区域
		control_doffocaldistance,	//焦距
		control_dofdepthbluramount, //深度模糊量
		control_dofdepthblurradius, //深度模糊半径
		control_bloomintensity,		//曝光强度
		control_bloomscale,			//曝光大小
		control_scenefringeintensity, //场景条纹强度
		control_COUNT				//控制数目
	};
	//类型 union
	typedef union {
		bool bool_val; //bool值
		float float_val;//浮点值
	} ansel_control_val;

private:
	void ReconfigureAnsel(); //重新配置
	void DeconfigureAnsel(); //删除配置
	//开始会话的回调
	static ansel::StartSessionStatus AnselStartSessionCallback(ansel::SessionConfiguration& settings, void* userPointer);
	//停止会话的回调
	static void AnselStopSessionCallback(void* userPointer);
	//开始抓取的回调
	static void AnselStartCaptureCallback(const ansel::CaptureConfiguration& CaptureInfo, void* userPointer);
	//停止抓取的回调
	static void AnselStopCaptureCallback(void* userPointer);
	//更改质量的回调
	static void AnselChangeQualityCallback(bool isHighQuality, void* userPointer);
	//相机匹配
	static bool AnselCamerasMatch(ansel::Camera& a, ansel::Camera& b);
	//Ansel相机至最小视图
	void AnselCameraToFMinimalView(FMinimalViewInfo& InOutPOV, ansel::Camera& AnselCam);
	//最小视图到 Ansel相机
	void FMinimalViewToAnselCamera(ansel::Camera& InOutAnselCam, FMinimalViewInfo& POV);
	//蓝图定义的相机
	bool BlueprintModifyCamera(ansel::Camera& InOutAnselCam, APlayerCameraManager* PCMgr); // returns whether modified cam is in original (session-start) position
	
	//配置渲染设置 为图片抓取
	void ConfigureRenderingSettingsForPhotography(FPostProcessSettings& InOutPostProcessSettings);
	//设置启动会话
	void SetUpSessionCVars();
	//启用自定义UI控制
	void DoCustomUIControls(FPostProcessSettings& InOutPPSettings, bool bRebuildControls);
	//申明滑条
	void DeclareSlider(int id, FText LocTextLabel, float LowerBound, float UpperBound, float Val);
	//后期UI滑条
	bool ProcessUISlider(int id, float& InOutVal);

	//抓取值
	bool CaptureCVar(FString CVarName);
	//设置抓取值的谓词（值名称，值如果没重置，函数 比较，想要重置，使用现有优先级）
	void SetCapturedCVarPredicated(const char* CVarName, float valueIfNotReset, std::function<bool(const float, const float)> comparison, bool wantReset, bool useExistingPriority);
	//设置抓取值（值名称，值如果不重置，想要重置=假，使用现有优先值=假）
	void SetCapturedCVar(const char* CVarName, float valueIfNotReset, bool wantReset = false, bool useExistingPriority = false);

	// ansel 的配置
	ansel::Configuration* AnselConfig;
	//ansel相机 Ansel相机
	ansel::Camera AnselCamera;
	//ansel相机 Ansel相机原点
	ansel::Camera AnselCameraOriginal;
	//ansel相机 ansel相机预览
	ansel::Camera AnselCameraPrevious;

	//最小视图信息 UE原有相机
	FMinimalViewInfo UECameraOriginal;
	//最小视图信息 UE相机预览
	FMinimalViewInfo UECameraPrevious;
	//后期设置 UE原有的后期设置
	FPostProcessSettings UEPostProcessingOriginal;

	bool bAnselSessionActive;						//ansel会话激活
	bool bAnselSessionNewlyActive;					//anselh会话新的激活
	bool bAnselSessionWantDeactivate;				//ansel会话取消激活
	bool bAnselCaptureActive;						//Ansel抓取激活
	bool bAnselCaptureNewlyActive;					//Ansel抓取激活
	bool bAnselCaptureNewlyFinished;				//Ansel抓取新的结束
	ansel::CaptureConfiguration AnselCaptureInfo;	//Ansel::抓取配置 ansel抓取信息

	/******   新增    ********/
	//是否进行下一轮的采集
	bool bTriggerNextCapture;
	//是否激活下一次的Tick
	bool bTriggerNextTick;

	//强制 禁止
	bool bForceDisallow;
	//是否 强制正射投影
	bool bIsOrthoProjection;

	//是否可以移动相机 在会话之前
	bool bWasMovableCameraBeforeSession;
	//是否暂停 在会话之前
	bool bWasPausedBeforeSession;
	// 是否显示HDR 在会话之前
	bool bWasShowingHUDBeforeSession;
	// b会话前是否启用字幕
	bool bWereSubtitlesEnabledBeforeSession;
	// 是否衰减启用会话之前
	bool bWasFadingEnabledBeforeSession;
	// 是否荧幕信息启动会话之前
	bool bWasScreenMessagesEnabledBeforeSession = false;
	// 时间膨胀会话
	float fTimeDilationBeforeSession;
	// 相机是否在原来的状态
	bool bCameraIsInOriginalState = true;

	//自动后期
	bool bAutoPostprocess;
	//自动暂停
	bool bAutoPause;
	//光追是不是被允许 = 不允许
	bool bRayTracingEnabled = false;
	//是否內部的暂停 =  假
	bool bPausedInternally = false;
	//高质量模式 
	bool bHighQualityModeDesired = false;
	//高质量模式 已经启用
	bool bHighQualityModeIsSetup = false;

	//ansel 的 FOV类型 = Ansel的水平FOV
	ansel::FovType RequiredFovType = ansel::kHorizontalFov;
	//ansel 的 Fov类型 当前配置Fov类型 = ansel 的 水平FOV
	ansel::FovType CurrentlyConfiguredFovType = ansel::kHorizontalFov;
	// 要求 世界的单位 =100
	float RequiredWorldToMeters = 100.f;
	//当前配置世界的 到米 = 0
	float CurrentlyConfiguredWorldToMeters = 0.f;

	//自会话启动后的帧数
	uint32_t NumFramesSinceSessionStart;

	// members relating to the 'Game Settings' controls in the Ansel overlay UI
	//与Ansel overlay UI中的“游戏设置”控件相关的成员
	TStaticBitArray<256> bEffectUIAllowed; //效果UI是否允许

	//UI控制需要重新构建
	bool bUIControlsNeedRebuild;
	//ansel 用户控制 UI控制【控制的数目】 这是一个数组
	ansel::UserControlDesc UIControls[control_COUNT];
	// ansel控制的值 UI控制值【数组】
	static ansel_control_val UIControlValues[control_COUNT]; // static to allow access from a callback
	//UI控制的低位数组
	float UIControlRangeLower[control_COUNT];
	//UI控制的高位数组
	float UIControlRangeUpper[control_COUNT];

	/** Console variable delegate for checking when the console variables have changed */
	// 控制太变量的代理 为查询  当 控制台变量被更改
	FConsoleCommandDelegate CVarDelegate;  //值的代理
	FConsoleVariableSinkHandle CVarDelegateHandle; //控制台变量的 下沉 句柄
	
	// 值的信息 
	struct CVarInfo {
		//控制台变量
		IConsoleVariable* cvar;
		//浮点的值
		float fInitialVal;
	};
	//初始画值的 键值对
	TMap<FString, CVarInfo> InitialCVarMap;
};
//ansel控制值   Ansel相机私类的 ：：UI控制值s【控制数目】
FNVAnselCameraPhotographyPrivate::ansel_control_val FNVAnselCameraPhotographyPrivate::UIControlValues[control_COUNT];
//AnselSDKDLL 的句柄指针
static void* AnselSDKDLLHandle = 0;
//Ansel的DLL是否被载入了
static bool bAnselDLLLoaded = false;

//NVAnsel相机图像抓取私有类：：抓取值（控制台值的名称）
bool FNVAnselCameraPhotographyPrivate::CaptureCVar(FString CVarName)
{
	//获取控制台的值的指针
	IConsoleVariable* cvar = IConsoleManager::Get().FindConsoleVariable(CVarName.GetCharArray().GetData());
	//空指针就跳出去
	if (!cvar) return false;
	//控制台指针信息 变量
	CVarInfo info;
	//信息的值 = 指针抓取来的值
	info.cvar = cvar;
	//信息,初始化的值 = 将值转化为浮点值
	info.fInitialVal = cvar->GetFloat();

	InitialCVarMap.Add(CVarName, info);
	return true;
}

// NVAnsel相机图像抓取私有类：：这个类的构造函数（）
FNVAnselCameraPhotographyPrivate::FNVAnselCameraPhotographyPrivate()
	: ICameraPhotography()		//相机的默认构造函数
	, bAnselSessionActive(false)  //默认的不激活
	, bAnselSessionNewlyActive(false)	//默认不开启激活
	, bAnselSessionWantDeactivate(false)	//会话是不是想要取消激活
	, bAnselCaptureActive(false)		//Ansel的抓取是否激活（否）	
	, bAnselCaptureNewlyActive(false)	//Ansel的抓取新的激活（否）
	, bAnselCaptureNewlyFinished(false)	//Ansel的抓取是否结束（否）
	, bTriggerNextCapture(false)
	, bTriggerNextTick(false)
	, bForceDisallow(false)				//强制禁用
	, bIsOrthoProjection(false)			//是否强制正射投影（否）
{
	//便利效果UI是否激活
	for (int i = 0; i < bEffectUIAllowed.Num(); ++i)
	{
		//默认全全部为开启
		bEffectUIAllowed[i] = true; // allow until explicitly disallowed 允许，直到明确禁止
	}
	//如果DLL没有载入
	if (bAnselDLLLoaded)
	{
		//新建一个Ansel的配置
		AnselConfig = new ansel::Configuration();
		//控制台值的代理 
		CVarDelegate = FConsoleCommandDelegate::CreateLambda([this] {
			//上一次的数值给重新赋值
			static float LastTranslationSpeed = -1.0f; //这是因为刚启动，必须将之前的记录重置
			static int32 LastSettleFrames = -1;
			//这一次的 移动速度 = 从控制台抓取过来的移动速度值
			float ThisTranslationSpeed = CVarPhotographyTranslationSpeed->GetFloat();
			//这一次的静态帧数 = 从控制台抓取过来的静态帧数值
			int32 ThisSettleFrames = CVarPhotographySettleFrames->GetInt();
			//这一次的移动速度、静态帧数和上一次的移动数值、静态帧数都不一样
			if (ThisTranslationSpeed != LastTranslationSpeed ||
				ThisSettleFrames != LastSettleFrames)
			{
				//重新配置Ansel
				ReconfigureAnsel();
				//然后将这两个值重新赋值
				LastTranslationSpeed = ThisTranslationSpeed;
				LastSettleFrames = ThisSettleFrames;
			}
		});
		//控制台值的代理的句柄 
		CVarDelegateHandle = IConsoleManager::Get().RegisterConsoleVariableSink_Handle(CVarDelegate);
		//重新配置代理
		ReconfigureAnsel();
	}
	else
	{
		//这里会打出（Ansel的DLL没有成功的启动）
		UE_LOG(LogAnsel, Log, TEXT("Ansel DLL was not successfully loaded."));
	}
}

//析构函数
FNVAnselCameraPhotographyPrivate::~FNVAnselCameraPhotographyPrivate()
{	
	if (bAnselDLLLoaded)
	{
		IConsoleManager::Get().UnregisterConsoleVariableSink_Handle(CVarDelegateHandle);
		DeconfigureAnsel();
		//这里将会删除Ansel的配置对象
		delete AnselConfig;
	}
}

//Ansel是否支持
bool FNVAnselCameraPhotographyPrivate::IsSupported()
{
	//根据两个标识位 并且 Ansel已经激活了没
	return bAnselDLLLoaded && ansel::isAnselAvailable();
}

//设置UI控制可见性（Ui控制的目标（枚举），是否可见）  //这个不用管这是第三放新增的意思
void FNVAnselCameraPhotographyPrivate::SetUIControlVisibility(uint8 UIControlTarget, bool bIsVisible)
{
	//这里的设置就是设置对应的UI效果是否可见（）
	bEffectUIAllowed[UIControlTarget] = bIsVisible;
}

//Ansel相机匹配（从相机b 匹配到相机a）
bool FNVAnselCameraPhotographyPrivate::AnselCamerasMatch(ansel::Camera& a, ansel::Camera& b)
{
	//主要有（位置，旋转，fov，映射偏移）
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

//Ansel相机到 “最小视窗”的转化工作（“最小视角信息” 作为输出，输入的是Ansel的相机）
void FNVAnselCameraPhotographyPrivate::AnselCameraToFMinimalView(FMinimalViewInfo& InOutPOV, ansel::Camera& AnselCam)
{
	//Fov值直接给
	InOutPOV.FOV = AnselCam.fov;
	//坐标位置直接给
	InOutPOV.Location.X = AnselCam.position.x;
	InOutPOV.Location.Y = AnselCam.position.y;
	InOutPOV.Location.Z = AnselCam.position.z;
	//旋转进行了一次转换
	FQuat rotq(AnselCam.rotation.x, AnselCam.rotation.y, AnselCam.rotation.z, AnselCam.rotation.w);
	//将旋转后的计算的值“rotq”，强转为FRotator后赋值
	InOutPOV.Rotation = FRotator(rotq);
	//调用相机的偏移函数（对偏移进行赋值）
	InOutPOV.OffCenterProjectionOffset.Set(AnselCam.projectionOffsetX, AnselCam.projectionOffsetY);
}

//小视窗到Ansel相机（输入输出的Ansel相机，最小视角信息 POV）
void FNVAnselCameraPhotographyPrivate::FMinimalViewToAnselCamera(ansel::Camera& InOutAnselCam, FMinimalViewInfo& POV)
{
	//Ansel相机
	InOutAnselCam.fov = POV.FOV;
	//Ansel位置
	InOutAnselCam.position = { float(POV.Location.X), float(POV.Location.Y), float(POV.Location.Z) };
	//旋转角转换
	FQuat rotq = POV.Rotation.Quaternion();
	InOutAnselCam.rotation = { float(rotq.X), float(rotq.Y), float(rotq.Z), float(rotq.W) };
	InOutAnselCam.projectionOffsetX = 0.f; // Ansel only writes these, doesn't read
	InOutAnselCam.projectionOffsetY = 0.f;
}

//蓝图定义相机（Ansel相机，玩家相机管理 玩家相机管理）
bool FNVAnselCameraPhotographyPrivate::BlueprintModifyCamera(ansel::Camera& InOutAnselCam, APlayerCameraManager* PCMgr)
{
	//最小视角  提出的
	FMinimalViewInfo Proposed;
	//Ansel相机到最小视角（提出的 输入输出相机）
	AnselCameraToFMinimalView(Proposed, InOutAnselCam);
	//玩家相机管理器(提出的视窗位置，相机预览.位置，UE原有相机.位置 ，提出的位置)
	PCMgr->PhotographyCameraModify(Proposed.Location, UECameraPrevious.Location, UECameraOriginal.Location, Proposed.Location/*out by ref*/);
	// only position has possibly changed
	// 仅当位置已经 可能更改
	InOutAnselCam.position.x = Proposed.Location.X;
	InOutAnselCam.position.y = Proposed.Location.Y;
	InOutAnselCam.position.z = Proposed.Location.Z;

	//相机预览 = 提出的
	UECameraPrevious = Proposed;

	//是否相机在原来的位移
	bool bIsCameraInOriginalTransform =
		Proposed.Location.Equals(UECameraOriginal.Location) &&
		Proposed.Rotation.Equals(UECameraOriginal.Rotation) &&
		Proposed.FOV == UECameraOriginal.FOV;
	return bIsCameraInOriginalTransform;
}

//声明 滑条（ID ,本地文字标签，最低边界，最高边界，值 ）
void FNVAnselCameraPhotographyPrivate::DeclareSlider(int id, FText LocTextLabel, float LowerBound, float UpperBound, float Val)
{
	//高边界和低边界
	UIControlRangeLower[id] = LowerBound;
	UIControlRangeUpper[id] = UpperBound;

	//UI控制[id].标签 = 字符串（本地文本标签.到字符串，获取字符串数组，获取数据）
	UIControls[id].labelUtf8 = TCHAR_TO_UTF8(LocTextLabel.ToString().GetCharArray().GetData());
	//UI控制值的【id】.浮点值 = 
	UIControlValues[id].float_val = FMath::GetRangePct(LowerBound, UpperBound, Val);
	//UI控制ID.回调 = [](用户控制信息 信息)
	UIControls[id].callback = [](const ansel::UserControlInfo& info) {
		//UI控制值【信息.】
		UIControlValues[info.userControlId - 1].float_val = *(float*)info.value;
	};
	//用户Id = id+1
	UIControls[id].info.userControlId = id + 1; // reserve 0 as 'unused'
	//用户控制类型 = Ansel的用户控制类型
	UIControls[id].info.userControlType = ansel::kUserControlSlider;
	//信息的值 = UI控制的值。浮点值
	UIControls[id].info.value = &UIControlValues[id].float_val;
	//用户控制状态  = Ansel的添加用户控制状态（UI控制【id】）
	ansel::UserControlStatus status = ansel::addUserControl(UIControls[id]);
	UE_LOG(LogAnsel, Log, TEXT("control#%d status=%d"), (int)id, (int)status);
}

//进程 UI 滑块
bool FNVAnselCameraPhotographyPrivate::ProcessUISlider(int id, float& InOutVal)
{
	//如果（UI控制【id】信息。用户控制ID）
	if (UIControls[id].info.userControlId <= 0)
	{
		return false; // control is not in use
	}

	InOutVal = FMath::Lerp(UIControlRangeLower[id], UIControlRangeUpper[id], UIControlValues[id].float_val);
	return true;
}

//进行自定义UI控制（后期设置 输入 ，重新构建控制）
void FNVAnselCameraPhotographyPrivate::DoCustomUIControls(FPostProcessSettings& InOutPPSettings, bool bRebuildControls)
{
	if (bRebuildControls)
	{
		// clear existing controls 清理存在的UI控制
		for (int i = 0; i < control_COUNT; ++i)
		{
			if (UIControls[i].info.userControlId > 0) // we are using id 0 as 'unused'
			{
				//移除用户控制
				ansel::removeUserControl(UIControls[i].info.userControlId);
				//UI控制ID，信息.用户控制ID
				UIControls[i].info.userControlId = 0;
			}
		}

		// save postproc settings at session start
		//在会话开始时保存程序后设置
		UEPostProcessingOriginal = InOutPPSettings;

		// add all relevant controls 添加所有关联控制
		PRAGMA_DISABLE_DEPRECATION_WARNINGS
		//是否开启深度场
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
		//如果效果允许【曝光】
		if (bEffectUIAllowed[Bloom] &&
			InOutPPSettings.BloomIntensity > 0.f)
		{
			DeclareSlider(
				control_bloomintensity,
				LOCTEXT("control_bloomintensity", "Bloom Intensity"),//曝光强度  
				0.f, 8.f,
				InOutPPSettings.BloomIntensity
			);
			DeclareSlider(
				control_bloomscale,
				LOCTEXT("control_bloomscale", "Bloom Scale"),//曝光缩放值
				0.f, 64.f,
				InOutPPSettings.BloomSizeScale
			);
		}

		if (bEffectUIAllowed[ChromaticAberration] &&
			InOutPPSettings.SceneFringeIntensity > 0.f)
		{
			DeclareSlider(
				control_scenefringeintensity,
				LOCTEXT("control_chromaticaberration", "Chromatic Aberration"),//色差
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
	// 后处理基于会话开始时间的后处理设置（避免由于摄像机在后处理卷之间徘徊而导致的 UI 可调整集发生变化，
	// 还避免了大多数不连续性，其中立体声和全景捕获也可能在捕获过程中在后处理卷之间徘徊）
	InOutPPSettings = UEPostProcessingOriginal;

	// update values where corresponding controls are in use
	//更新正在使用相应控件的值
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

//更新相机（相机视窗，相机管理器）
//这个是每一帧都在干的事
bool FNVAnselCameraPhotographyPrivate::UpdateCamera(FMinimalViewInfo& InOutPOV, APlayerCameraManager* PCMgr)
{
	check(PCMgr != nullptr);
	
	bool bGameCameraCutThisFrame = false;
	//是否强制禁止
	bForceDisallow = false; 
	if (!bAnselSessionActive)
	{
		//获取并存储一些影响 Ansel 会话设置的视图详细信息，
		//但从 Ansel 回调（不一定在渲染或游戏线程上）访问这些视图详细信息可能不安全
		bIsOrthoProjection = (InOutPOV.ProjectionMode == ECameraProjectionMode::Orthographic);
		//视图的客户端 指针 = 相机管理 获取世界——获取游视窗
		//这里禁止是分屏状态下工作
		if (UGameViewportClient* ViewportClient = PCMgr->GetWorld()->GetGameViewport())
		{
			// 强制禁止 = 强制静止或者 视窗客户端- 获取当前分屏的配置 （不等于） 分屏类型不为空
			bForceDisallow = bForceDisallow || (ViewportClient->GetCurrentSplitscreenConfiguration() != ESplitScreenType::None); // forbid if in splitscreen.
		}

		// 禁止图像抓取 如果在 沉浸模式或者VR模式
		bForceDisallow = bForceDisallow || (GEngine->IsStereoscopic3D());

		// 不断检查某些游戏参数中不经常更改的内容，这很烦人 要求完全重新初始化 Ansel
		// 1.删除 世界到米的缩放

		// 如果（获取世界的指针 并且不为空）没用
		if (const UWorld* World = GEngine->GetWorld())
		{
			
			//拿到世界设置  世界设置 = 引擎 获取世界 获取世界设置
			const AWorldSettings* WorldSettings = GEngine->GetWorld()->GetWorldSettings();
			//世界设置 并且 世界设置的缩放 不等于0 
			if (WorldSettings && WorldSettings->WorldToMeters != 0.f)
			{
				//请求的世界缩放 = 世界设置 世界缩放
				RequiredWorldToMeters = WorldSettings->WorldToMeters;
			}
		}
		
		// 检测 FOV 约束设置 - 对于多部分快照切片至关重要	
		//玩家控制 PC = 玩家相机管理 拿到 玩家的控制器 这个控制器不为空
		if (const APlayerController* PC = PCMgr->GetOwningPlayerController())
		{
			// 如果 本地玩家 = 玩家控制器 获取的本地玩家不为空
			if (const ULocalPlayer* LocalPlayer = PC->GetLocalPlayer())
			{
				// 游戏视窗的代理  视窗代理 = 本地的玩家 的视窗代理  不为空
				if (const UGameViewportClient* ViewportClient = LocalPlayer->ViewportClient)
				{
					// 视窗 = 视窗代理 获取到视窗
					if (const FViewport* Viewport = ViewportClient->Viewport)
					{
						// 二维向量 视图缩放 = 本地 尺寸
						const FVector2D& LPViewScale = LocalPlayer->Size;
						// 尺寸 X =
						// 尺寸 Y = 
						uint32 SizeX = FMath::TruncToInt(LPViewScale.X * Viewport->GetSizeXY().X);
						uint32 SizeY = FMath::TruncToInt(LPViewScale.Y * Viewport->GetSizeXY().Y);

						// 纵横比轴约束的·枚举 = 本地玩家 - 纵横比轴约束
						EAspectRatioAxisConstraint AspectRatioAxisConstraint = LocalPlayer->AspectRatioAxisConstraint;

						// (logic from FMinimalViewInfo::CalculateProjectionMatrixGivenView() -) if x is bigger, and we're respecting x or major axis, AND mobile isn't forcing us to be Y axis aligned
						// 如果（尺寸X>尺寸Y） 或者 （宽幅比 == 宽幅比 主轴上的FOV） 或者 （纵横比轴约束 == 宽幅比 主容器XFOV） 或者 相机的模式 == 枚举矩阵模式的正交模式）
						if (((SizeX > SizeY) && (AspectRatioAxisConstraint == AspectRatio_MajorAxisFOV)) || (AspectRatioAxisConstraint == AspectRatio_MaintainXFOV) || (InOutPOV.ProjectionMode == ECameraProjectionMode::Orthographic))
						{
							//请求FOV类型 = 水平FOV
							RequiredFovType = ansel::kHorizontalFov;
						}
						else
						{
							//水平FOV
							RequiredFovType = ansel::kVerticalFov;
						}
					}
				}
			}
		}
		// 如果 （当前配置世界的米 不等于 请求世界的米 或者 当前世界的FOV发生了变化的时候）
		if (CurrentlyConfiguredWorldToMeters != RequiredWorldToMeters ||
			CurrentlyConfiguredFovType != RequiredFovType)
		{
			//需要重新配置Ansel
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
			//加这里试试
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
				//添加函数调用，这样可以避免Sequnce偷跑
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


//设置抓取的值的断言(C类的值，浮点 没有重置的值 ，对比，重置，使用优先级)
void FNVAnselCameraPhotographyPrivate::SetCapturedCVarPredicated(const char* CVarName, float valueIfNotReset, std::function<bool(const float, const float)> comparison, bool wantReset, bool useExistingPriority)
{
	//控制台值的指针
	CVarInfo* info = nullptr;
	// 初始化C值的映射。包含（C值名）|| 抓取C值（C值名）
	if (InitialCVarMap.Contains(CVarName) || CaptureCVar(CVarName))
	{
		//信息 = 初始化C值Map
		info = &InitialCVarMap[CVarName];
		//如果 （信息-C值 对比 对比（如果值没有重置） 信息 - 初始化的值）
		if (info->cvar && comparison(valueIfNotReset, info->fInitialVal))
		{
			//使用已经存在的优先级
			if (useExistingPriority)
				//信息 值 - 设置 当前优先级（想要重置吗？ 从信息拿到的初始化）
				info->cvar->SetWithCurrentPriority(wantReset ? info->fInitialVal : valueIfNotReset);
			else
				//信息 值 设置（想要重置 ？ 信息 初始化的值 ： 没有重置的值）
				info->cvar->Set(wantReset ? info->fInitialVal : valueIfNotReset);
		}
	}
	if (!(info && info->cvar)) UE_LOG(LogAnsel, Log, TEXT("CVar used by Ansel not found: %s"), CVarName);
}

//设置抓取的C值（常量 C值的名称 ，没有重置的值 ，是否想要重置，是否使用存在的优先级）
void FNVAnselCameraPhotographyPrivate::SetCapturedCVar(const char* CVarName, float valueIfNotReset, bool wantReset, bool useExistingPriority)
{
	//这里干了一件事（将一些变量省去一些，省区对比值）
	SetCapturedCVarPredicated(CVarName, valueIfNotReset,
		[](float, float) { return true; },
		wantReset, useExistingPriority);
}


//配置渲染设置为图片抓取（后期设置）
void FNVAnselCameraPhotographyPrivate::ConfigureRenderingSettingsForPhotography(FPostProcessSettings& InOutPostProcessingSettings)
{
//定义一段宏定义	
// 质量_C值（名字，值）  
#define QUALITY_CVAR(NAME,BOOSTVAL) SetCapturedCVar(NAME, BOOSTVAL, !bHighQualityModeDesired, true)
// 质量的最低值
#define QUALITY_CVAR_AT_LEAST(NAME,BOOSTVAL) SetCapturedCVarPredicated(NAME, BOOSTVAL, std::greater<float>(), !bHighQualityModeDesired, true)
// 质量的最高值
#define QUALITY_CVAR_AT_MOST(NAME,BOOSTVAL) SetCapturedCVarPredicated(NAME, BOOSTVAL, std::less<float>(), !bHighQualityModeDesired, true)
// 质量最低优先级的值
#define QUALITY_CVAR_LOWPRIORITY_AT_LEAST(NAME,BOOSTVAL) SetCapturedCVarPredicated(NAME, BOOSTVAL, std::greater<float>(), !bHighQualityModeDesired, false)
	
	// 如果C值允许最高质量.获取任意线程上的值
	if (CVarAllowHighQuality.GetValueOnAnyThread()
		//并且 高质量模式设置 ！= 高质量模式
		&& bHighQualityModeIsSetup != bHighQualityModeDesired
		// (被内部暂停或者 非自动暂停)
		&& (bPausedInternally || !bAutoPause) // <- don't start overriding vars until truly paused
		)
	{
		// Pump up (or reset) the quality. 

		// bring rendering up to (at least) 100% resolution, but won't override manually set value on console
		// 截图的最低质量
		QUALITY_CVAR_LOWPRIORITY_AT_LEAST("r.ScreenPercentage", 100);

		// most of these are similar to typical cinematic sg.* scalability settings, toned down a little for performance

		// can be a mild help with reflections quality
		// 反射质量
		QUALITY_CVAR("r.gbufferformat", 5); // 5 = highest precision

		// bias various geometry LODs 
		// 模型LOD质量
		QUALITY_CVAR_AT_MOST("r.staticmeshloddistancescale", 0.25f); // large quality bias
		QUALITY_CVAR_AT_MOST("r.landscapelodbias", -2);
		QUALITY_CVAR_AT_MOST("r.skeletalmeshlodbias", -4); // big bias here since when paused this never gets re-evaluated and the camera could roam to look at a skeletal mesh far away

		// ~sg.AntiAliasingQuality @ cine 
		// 抗锯齿质量
		QUALITY_CVAR("r.postprocessaaquality", 6); // 6 == max
		QUALITY_CVAR_AT_LEAST("r.defaultfeature.antialiasing", 2); // TAA or higher
		QUALITY_CVAR_AT_LEAST("r.ngx.dlss.quality", 2); // high-quality mode for DLSS if in use

		// ~sg.EffectsQuality @ cinematic
		// 效果质量
		QUALITY_CVAR_AT_LEAST("r.TranslucencyLightingVolumeDim", 64);
		QUALITY_CVAR("r.RefractionQuality", 2);
		QUALITY_CVAR("r.SSR.Quality", 4);
		// QUALITY_CVAR("r.SceneColorFormat", 4); // no - don't really want to mess with this
		QUALITY_CVAR("r.TranslucencyVolumeBlur", 1);
		//材质质量
		QUALITY_CVAR("r.MaterialQualityLevel", 1); // 0==low, -> 1==high <- , 2==medium
		QUALITY_CVAR("r.SSS.Scale", 1);
		QUALITY_CVAR("r.SSS.SampleSet", 2);
		QUALITY_CVAR("r.SSS.Quality", 1);
		QUALITY_CVAR("r.SSS.HalfRes", 0);
		//QUALITY_CVAR_AT_LEAST("r.EmitterSpawnRateScale", 1.f); // no - not sure this has a point when game is paused
		QUALITY_CVAR("r.ParticleLightQuality", 2);
		QUALITY_CVAR("r.DetailMode", 2);

		// ~sg.PostProcessQuality @ cinematic
		// 后期质量
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
		// 贴图质量 @影视级
		QUALITY_CVAR("r.Streaming.MipBias", 0);
		QUALITY_CVAR_AT_LEAST("r.MaxAnisotropy", 16);
		QUALITY_CVAR("r.Streaming.MaxEffectiveScreenSize", 0);
		// intentionally don't mess with streaming pool size, see 'CVarExtreme' section below

		// ~sg.FoliageQuality @ cinematic 
		// 植被质量
		QUALITY_CVAR_AT_LEAST("foliage.DensityScale", 1.f);
		QUALITY_CVAR_AT_LEAST("grass.DensityScale", 1.f);
		// boosted foliage LOD (use distance scale not lod bias - latter is buggy)
		// Lod 距离缩放改为之前的四倍
		QUALITY_CVAR_AT_LEAST("foliage.LODDistanceScale", 4.f);

		// ~sg.ViewDistanceQuality @ cine but only mild draw distance boost
		QUALITY_CVAR_AT_LEAST("r.viewdistancescale", 2.0f); // or even more...?

		// ~sg.ShadowQuality @ cinematic //灯光阴影质量
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
		// 如果光追开启后 启用下面的试着参数
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
			// 高质量模式下的设置
			QUALITY_CVAR_AT_LEAST("r.RayTracing.Translucency.MaxRoughness", 0.9f);
			QUALITY_CVAR_AT_LEAST("r.RayTracing.Translucency.MaxRayDistance", 1000000.f);
			QUALITY_CVAR_AT_LEAST("r.RayTracing.Translucency.MaxRefractionRays", 11); // number of layers of ray penetration, actually regardless of whether refraction is enabled
			QUALITY_CVAR_AT_LEAST("r.RayTracing.Translucency.Shadows", 1); // turn on at least basic quality

			//高质量模式
			/*** HIGH-QUALITY MODE DOES *NOT* FORCE RT SHADOWS ON ***/
			QUALITY_CVAR("r.RayTracing.Shadow.MaxLights", -1); // unlimited
			QUALITY_CVAR("r.RayTracing.Shadow.MaxDenoisedLights", -1); // unlimited

			// these apply to various RT effects but mostly reflections+translucency
			// 应用光追效果 的反射+半透明
			QUALITY_CVAR_AT_LEAST("r.raytracing.lighting.maxshadowlights", 256); // as seen in reflections/translucencies
			QUALITY_CVAR_AT_LEAST("r.RayTracing.lighting.maxlights", 256); // as seen in reflections/translucencies
		}

		 // these are some extreme settings whose quality:risk ratio may be debatable or unproven
		 // 这些是一些极端设置，其质量：风险比可能值得商榷或未经证实
		if (CVarExtreme->GetInt())
		{
			// great idea but not until I've proven that this isn't deadly or extremely slow on lower-spec machines:
			// 好主意，但直到我证明这在低规格机器上不是致命的或非常慢的：
			QUALITY_CVAR("r.Streaming.LimitPoolSizeToVRAM", 0); // 0 is aggressive but is it safe? seems safe.
			QUALITY_CVAR_AT_LEAST("r.Streaming.PoolSize", 3000); // cine - perhaps redundant when r.streaming.fullyloadusedtextures

			QUALITY_CVAR("r.streaming.hlodstrategy", 2); // probably use 0 if using r.streaming.fullyloadusedtextures, else 2
			//QUALITY_CVAR("r.streaming.fullyloadusedtextures", 1); // no - LODs oscillate when overcommitted
			QUALITY_CVAR_AT_LEAST("r.viewdistancescale", 10.f); // cinematic - extreme

			//光追开启的情况下开启以下参数
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
			//只是没有经过大量测试：
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
	//如果Ansel抓取激活
	if (bAnselCaptureActive)
	{
		// camera doesn't linger in one place very long so maximize streaming rate
		SetCapturedCVar("r.disablelodfade", 1);
		SetCapturedCVar("r.streaming.framesforfullupdate", 1); // recalc required LODs ASAP
		SetCapturedCVar("r.Streaming.MaxNumTexturesToStreamPerFrame", 0); // no limit
		SetCapturedCVar("r.streaming.numstaticcomponentsprocessedperframe", 0); // 0 = load all pending static geom now
		//如果开启自动后期
		if (bAutoPostprocess)
		{
			// force-disable the standard postprocessing effects which are known to
			// be problematic in multi-part shots

			// nerf remaining motion blur 运动模糊控制
			InOutPostProcessingSettings.bOverride_MotionBlurAmount = 1;
			InOutPostProcessingSettings.MotionBlurAmount = 0.f;

			// these effects tile poorly   这些效果平铺效果差
			InOutPostProcessingSettings.bOverride_BloomDirtMaskIntensity = 1;
			InOutPostProcessingSettings.BloomDirtMaskIntensity = 0.f;
			InOutPostProcessingSettings.bOverride_LensFlareIntensity = 1;
			InOutPostProcessingSettings.LensFlareIntensity = 0.f;
			InOutPostProcessingSettings.bOverride_VignetteIntensity = 1;
			InOutPostProcessingSettings.VignetteIntensity = 0.f;
			InOutPostProcessingSettings.bOverride_SceneFringeIntensity = 1;
			InOutPostProcessingSettings.SceneFringeIntensity = 0.f;

			// freeze auto-exposure adaptation 
			// 冻结自动曝光适配
			InOutPostProcessingSettings.bOverride_AutoExposureSpeedDown = 1;
			InOutPostProcessingSettings.AutoExposureSpeedDown = 0.f;
			InOutPostProcessingSettings.bOverride_AutoExposureSpeedUp = 1;
			InOutPostProcessingSettings.AutoExposureSpeedUp = 0.f;

			// bring rendering up to (at least) full resolution
			// 带来渲染提升至少全分辨率
			if (InOutPostProcessingSettings.ScreenPercentage_DEPRECATED < 100.f)
			{
				// note: won't override r.screenpercentage set from console, that takes precedence
				InOutPostProcessingSettings.bOverride_ScreenPercentage_DEPRECATED = 1;
				InOutPostProcessingSettings.ScreenPercentage_DEPRECATED = 100.f;
			}
			// Ansel抓取激活 = 抓取信息.抓取类型 等于 超高分辨率抓取
			bool bAnselSuperresCaptureActive = AnselCaptureInfo.captureType == ansel::kCaptureTypeSuperResolution;
			// 如果抓取类型是 360立体 或者 是立体模式
			bool bAnselStereoCaptureActive = AnselCaptureInfo.captureType == ansel::kCaptureType360Stereo || AnselCaptureInfo.captureType == ansel::kCaptureTypeStereo;
			if (bAnselStereoCaptureActive)
			{
				// Attempt to nerf DoF in stereoscopic shots where it can be quite unpleasant for the viewer
				// 尝试在立体镜头中nerf DoF，这对观众来说可能非常不愉快
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
			//不是ansel超级抓取激活
			if (!bAnselSuperresCaptureActive)
			{
				// Disable SSR in multi-part shots unless taking a super-resolution shot; SSR *usually* degrades gracefully in tiled shots, and super-resolution mode in Ansel has an 'enhance' option which repairs any lingering SSR artifacts quite well.
				InOutPostProcessingSettings.bOverride_ScreenSpaceReflectionIntensity = 1;
				InOutPostProcessingSettings.ScreenSpaceReflectionIntensity = 0.f;
			}
		}
	}
}

//启用 会话C值
void FNVAnselCameraPhotographyPrivate::SetUpSessionCVars()
{
	// This set of CVar tweaks are good - or necessary - for photographic sessions
	// 这组CVar调整对于摄影会议来说是好的 - 或者是必要的
	{
		SetCapturedCVar("r.oneframethreadlag", 1); // ansel needs frame latency to be predictable ansel 需要帧延迟才能可预测

		// these are okay tweaks to streaming heuristics to reduce latency of full texture loads or minimize VRAM waste
		// 这些都是对流式启发式的调整，以减少完整纹理加载的延迟或最大限度地减少VRAM浪费
		// 严格优先考虑当前可见的内容
		SetCapturedCVar("r.streaming.minmipforsplitrequest", 1); // strictly prioritize what's visible right now 
		//提示引擎降低模糊纹理的优先级...？
		SetCapturedCVar("r.streaming.hiddenprimitivescale", 0.001f); // hint to engine to deprioritize obscured textures...?
		SetCapturedCVar("r.Streaming.Boost", 1);
		//此 nerfs 运动模糊用于非字符
		SetCapturedCVar("r.motionblurquality", 0); // this nerfs motion blur for non-characters
	}
}

//更新后期设置（后期设置 输入输出后期设置）
void FNVAnselCameraPhotographyPrivate::UpdatePostProcessing(FPostProcessSettings& InOutPostProcessingSettings)
{
	// ansel会话激活
	if (bAnselSessionActive)
	{
		//启用自定义UI控制（用于输入输出的后期设置，是否启用UI控制的重构）
		DoCustomUIControls(InOutPostProcessingSettings, bUIControlsNeedRebuild);
		//配置渲染设置（用于输入输出的后期设置）
		ConfigureRenderingSettingsForPhotography(InOutPostProcessingSettings);
	}
}

// 开启会话
void FNVAnselCameraPhotographyPrivate::StartSession()
{
	ansel::startSession();
}

// 停止会话
void FNVAnselCameraPhotographyPrivate::StopSession()
{
	ansel::stopSession();
}

//默认约束摄像机(新的相机位置，预览相机位置，原来相机位置，输出相机位置 ，玩家相机控制)
void FNVAnselCameraPhotographyPrivate::DefaultConstrainCamera(const FVector NewCameraLocation, const FVector PreviousCameraLocation, const FVector OriginalCameraLocation, FVector& OutCameraLocation, APlayerCameraManager* PCMgr)
{
	// let proposed camera through unmodified by default
	// 让建议的相机通过未修改默认情况下
	OutCameraLocation = NewCameraLocation;

	// First, constrain by distance 通过距离约束
	FVector ConstrainedLocation;
	// 最大距离
	float MaxDistance = CVarConstrainCameraDistance->GetFloat();
	//通过距离约束相机
	UAnselFunctionLibrary::ConstrainCameraByDistance(PCMgr, NewCameraLocation, PreviousCameraLocation, OriginalCameraLocation, ConstrainedLocation, MaxDistance);

	// Second, constrain against collision geometry 约束碰撞几何图形
	// 第二步 通过模型约束相机
	UAnselFunctionLibrary::ConstrainCameraByGeometry(PCMgr, ConstrainedLocation, PreviousCameraLocation, OriginalCameraLocation, OutCameraLocation);
}

// 开启会话的状态 Ansel开启会话回调（Ansel会话配置 ，用户指针）
ansel::StartSessionStatus FNVAnselCameraPhotographyPrivate::AnselStartSessionCallback(ansel::SessionConfiguration& settings, void* userPointer)
{
	//ansel 开启会话状态   将会话状态改为  disallow （不允许）
	ansel::StartSessionStatus AnselSessionStatus = ansel::kDisallowed;
	//私有实例 = 静态转换（用户指针）
	FNVAnselCameraPhotographyPrivate* PrivateImpl = static_cast<FNVAnselCameraPhotographyPrivate*>(userPointer);
	//检查指针非空
	check(PrivateImpl != nullptr);
	// 实例 不是强制禁用 && 图像抓取获取的整数 && 不是编辑器状态
	if (!PrivateImpl->bForceDisallow && CVarPhotographyAllow->GetInt() && !GIsEditor)
	{
		//暂停允许 
		bool bPauseAllowed = true;
		//禁用多部分 = ！！图像抓取多部分 - 获取整数
		bool bEnableMultipart = !!CVarPhotographyEnableMultipart->GetInt();

		// 允许移动 
		settings.isTranslationAllowed = true;
		settings.isFovChangeAllowed = !PrivateImpl->bIsOrthoProjection;   //正交相机就不支持移动
		settings.isRotationAllowed = true;								//是否允许旋转
		settings.isPauseAllowed = bPauseAllowed;						//bPauseAllowed//是否允许暂停
		settings.isHighresAllowed = bEnableMultipart;								//bEnableMultipart	//是否允许高分辨率
		settings.is360MonoAllowed = bEnableMultipart;					//是否允许360模式
		settings.is360StereoAllowed = bEnableMultipart;							//bEnableMultipart //是否允许360立体

		PrivateImpl->bAnselSessionActive = true;						//Ansel 会话激活
		PrivateImpl->bAnselSessionNewlyActive = true;					//Ansel新的会话激活
		PrivateImpl->bHighQualityModeDesired = false;					// 不是高质量模式

		AnselSessionStatus = ansel::kAllowed;							//Ansel的会话状态改为允许
	}

	UE_LOG(LogAnsel, Log, TEXT("Photography camera session attempt started, Allowed=%d, ForceDisallowed=%d"), int(AnselSessionStatus == ansel::kAllowed), int(PrivateImpl->bForceDisallow));
	
	//Ansel的状态
	return AnselSessionStatus;
}

void FNVAnselCameraPhotographyPrivate::AnselStopSessionCallback(void* userPointer)
{
	//
	FNVAnselCameraPhotographyPrivate* PrivateImpl = static_cast<FNVAnselCameraPhotographyPrivate*>(userPointer);
	//检查实例是否为空
	check(PrivateImpl != nullptr);
	//私有实例 （Ansel会话激活了&&实例 Ansel会话新的激活）
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

//Ansel 开始捕获的回调函数（常量 ansel抓取的配置信息，用户指针）
void FNVAnselCameraPhotographyPrivate::AnselStartCaptureCallback(const ansel::CaptureConfiguration& CaptureInfo, void* userPointer)
{
	//查找对象实例
	FNVAnselCameraPhotographyPrivate* PrivateImpl = static_cast<FNVAnselCameraPhotographyPrivate*>(userPointer);
	check(PrivateImpl != nullptr);
	//实例抓取激活标识开启
	PrivateImpl->bAnselCaptureActive = true;
	//实例的抓取新的激活
	PrivateImpl->bAnselCaptureNewlyActive = true;
	//实例的抓取信息 = ansel的抓取信息
	PrivateImpl->AnselCaptureInfo = CaptureInfo;

	UE_LOG(LogAnsel, Log, TEXT("Photography camera multi-part capture started"));
}

//这个回调可以让用户知道截图已经完成
void FNVAnselCameraPhotographyPrivate::AnselStopCaptureCallback(void* userPointer)
{
	FNVAnselCameraPhotographyPrivate* PrivateImpl = static_cast<FNVAnselCameraPhotographyPrivate*>(userPointer);
	check(PrivateImpl != nullptr);
	//实例抓取关闭  = 假
	PrivateImpl->bAnselCaptureActive = false;
	//实例抓取 新的关闭为真
	PrivateImpl->bAnselCaptureNewlyFinished = true;  //@@@@重要

	UE_LOG(LogAnsel, Log, TEXT("Photography camera multi-part capture end"));
}

//Ansel更改质量的回调（是否高质量模式，一个Cpp用户的指针）
void FNVAnselCameraPhotographyPrivate::AnselChangeQualityCallback(bool isHighQuality, void* ACPPuserPointer)
{
	FNVAnselCameraPhotographyPrivate* PrivateImpl = static_cast<FNVAnselCameraPhotographyPrivate*>(ACPPuserPointer);
	check(PrivateImpl != nullptr);
	//实例对象的是否高质量模式 被渴望 = 是否高质量模式
	PrivateImpl->bHighQualityModeDesired = isHighQuality;

	UE_LOG(LogAnsel, Log, TEXT("Photography HQ mode toggle (%d)"), (int)isHighQuality);
}

//这个也很重要
void FNVAnselCameraPhotographyPrivate::ReconfigureAnsel()
{
	check(AnselConfig != nullptr);
	//ansel配置 - 用户指针 = 本体
	AnselConfig->userPointer = this;
	//各种回调函数的指针
	AnselConfig->startSessionCallback = AnselStartSessionCallback;
	AnselConfig->stopSessionCallback = AnselStopSessionCallback;
	AnselConfig->startCaptureCallback = AnselStartCaptureCallback;
	AnselConfig->stopCaptureCallback = AnselStopCaptureCallback;
	AnselConfig->changeQualityCallback = AnselChangeQualityCallback;

	// Getting fovType wrong can lead to multi-part captures stitching incorrectly, especially 360 shots
	// fovType错误可能导致多部分捕获拼接不正确，尤其是360度拍摄
	AnselConfig->fovType = RequiredFovType;
	// 引擎 - 游戏视图    并且    引擎游戏视图获取窗口有值   并且  引擎获取游戏视窗-获取原生窗口有值
	if (GEngine->GameViewport && GEngine->GameViewport->GetWindow().IsValid() && GEngine->GameViewport->GetWindow()->GetNativeWindow().IsValid())
	{
		//ansel配置的游戏视窗句柄 = 引擎游戏视窗的窗口下的原生窗口 然后获取os窗口的句柄
		AnselConfig->gameWindowHandle = GEngine->GameViewport->GetWindow()->GetNativeWindow()->GetOSWindowHandle();
	}
	UE_LOG(LogAnsel, Log, TEXT("gameWindowHandle= %p"), AnselConfig->gameWindowHandle);
	//ansel配置  —— 移动速度在世界单位中每秒  = 从控制台变量 中的移动速度 获取的浮点值
	AnselConfig->translationalSpeedInWorldUnitsPerSecond = CVarPhotographyTranslationSpeed->GetFloat();

	//ansel配置 米在世界单位 = 1/请求世界到米
	AnselConfig->metersInWorldUnit = 1.0f / RequiredWorldToMeters;
	UE_LOG(LogAnsel, Log, TEXT("We reckon %f meters to 1 world unit"), AnselConfig->metersInWorldUnit);
	//相机移轴是否支持
	AnselConfig->isCameraOffcenteredProjectionSupported = true;
	//捕获相关性
	AnselConfig->captureLatency = 0; // important
	//捕获稳定延迟
	AnselConfig->captureSettleLatency = CVarPhotographySettleFrames->GetInt();


	//ansel 设置配置状态 状态 = Ansell;;设置配置（*ansel配置）
	ansel::SetConfigurationStatus status = ansel::setConfiguration(*AnselConfig);
	if (status != ansel::kSetConfigurationSuccess)
	{
		UE_LOG(LogAnsel, Log, TEXT("ReconfigureAnsel setConfiguration returned %ld"), (long int)(status));
	}
	//当前配置FOV类型 = 请求FOV类型
	CurrentlyConfiguredFovType = RequiredFovType;
	//当前配置世界到米 =请求世界到米
	CurrentlyConfiguredWorldToMeters = RequiredWorldToMeters;
}

//清空Ansel的配置
void FNVAnselCameraPhotographyPrivate::DeconfigureAnsel()
{
	//检查Ansel配置不为空
	check(AnselConfig != nullptr);
	//Ansel配置的各种回调清空
	AnselConfig->userPointer = nullptr;
	AnselConfig->startSessionCallback = nullptr;
	AnselConfig->stopSessionCallback = nullptr;
	AnselConfig->startCaptureCallback = nullptr;
	AnselConfig->stopCaptureCallback = nullptr;
	AnselConfig->gameWindowHandle = nullptr;
	// ansel 的设置配置状态  =  Ansel的设置配置（*Ansel配置）
	ansel::SetConfigurationStatus status = ansel::setConfiguration(*AnselConfig);
	//状态为配置非成功的状态
	if (status != ansel::kSetConfigurationSuccess)
	{
		UE_LOG(LogAnsel, Log, TEXT("DeconfigureAnsel setConfiguration returned %ld"), (long int)(status));
	}
}


//Ansel的模块 继承自 Ansel的接口类
class FAnselModule : public IAnselModule
{
public:
	//启动模块
	virtual void StartupModule() override
	{
		//相机捕获模块启动
		ICameraPhotographyModule::StartupModule();
		//Ansel的Dll是否已经载入
		check(!bAnselDLLLoaded);

		// Late-load Ansel DLL.  DLL name has been worked out by the build scripts as ANSEL_DLL
		FString AnselDLLName;	//AnselDll的名字

		//Ansel的发布文件根目录 = 
		FString AnselBinariesRoot = FPaths::ProjectPluginsDir() / TEXT("Ansel/Binaries/ThirdParty/");
		// common preprocessor fudge to convert macro expansion into string
		// 用于将宏扩展转换为字符串的常见预处理器软糖
#define STRINGIFY(X) STRINGIFY2(X)
#define STRINGIFY2(X) #X
			AnselDLLName = AnselBinariesRoot + TEXT(STRINGIFY(ANSEL_DLL));
			AnselSDKDLLHandle = FPlatformProcess::GetDllHandle(*(AnselDLLName));
			//anselDll载入  = AnselSDK句柄不为空
		bAnselDLLLoaded = AnselSDKDLLHandle != 0;
		UE_LOG(LogAnsel, Log, TEXT("Tried to load %s : success=%d"), *AnselDLLName, int(bAnselDLLLoaded));
	}
	//关闭模块
	virtual void ShutdownModule() override
	{		
		if (bAnselDLLLoaded)
		{
			//释放句柄
			FPlatformProcess::FreeDllHandle(AnselSDKDLLHandle);
			//释放指针
			AnselSDKDLLHandle = 0;
			//更改载入状态
			bAnselDLLLoaded = false;
		}
		//相机捕获模块关闭
		ICameraPhotographyModule::ShutdownModule();
	}
private:
	// 返回相机捕获的接口类 创建相机（）重载
	virtual TSharedPtr< class ICameraPhotography > CreateCameraPhotography() override
	{
		//定义一个共享指针
		TSharedPtr<ICameraPhotography> Photography = nullptr;

		//实例化一个 图像捕获的的实例
		FNVAnselCameraPhotographyPrivate* PhotographyPrivate = new FNVAnselCameraPhotographyPrivate();
		if (PhotographyPrivate->IsSupported())
		{
			//类型的强转
			Photography = TSharedPtr<ICameraPhotography>(PhotographyPrivate);
		}
		else
		{
			delete PhotographyPrivate;
		}
		//返回这个子类的对象
		return Photography;
	}
};
//实例模块的（模块类名，模块名）
IMPLEMENT_MODULE(FAnselModule, Ansel)

#undef LOCTEXT_NAMESPACE