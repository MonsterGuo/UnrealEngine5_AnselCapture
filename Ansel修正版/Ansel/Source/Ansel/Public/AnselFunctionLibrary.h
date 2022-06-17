// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "Kismet/BlueprintFunctionLibrary.h"
#include "AnselFunctionLibrary.generated.h"

UENUM(BlueprintType)
//枚举 控制效果 目标
enum EUIControlEffectTarget
{
	Bloom,
	DepthOfField,
	ChromaticAberration,
	MotionBlur
};

UCLASS()
// 蓝图函数模块
class ANSEL_API UAnselFunctionLibrary : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:
	/** Starts a photography session */  //开始一个图形抓取会话
	// UObject* WorldContextObject 任何对象都可以使用
	UFUNCTION(BlueprintCallable, Category = "Photography", meta = (WorldContext = WorldContextObject))
	static void StartSession(UObject* WorldContextObject);

	/** Stops a photography session */	//结束一个图形抓取会话
	UFUNCTION(BlueprintCallable, Category = "Photography", meta = (WorldContext = WorldContextObject))
	static void StopSession(UObject* WorldContextObject);

	/** Whether the photography system is available at all.  See CVar r.Photography.Available */
	// 摄影系统是否可用。 请参阅 CVar r.Photography.Available
	UFUNCTION(BlueprintCallable, Category = "Photography")
	static bool IsPhotographyAvailable();

	/** Whether the app is permitting photography at this time.  See CVar r.Photography.Allowed */
	// 应用程序此时是否允许拍照。 请参阅 CVar r.Photography.Allowed
	UFUNCTION(BlueprintCallable, Category = "Photography")
	static bool IsPhotographyAllowed();

	/** Sets whether the app is permitting photography at this time.  See CVar r.Photography.Allowed */
	// 设置应用程序此时是否允许拍照。 请参阅 CVar r.Photography.Allowed
	UFUNCTION(BlueprintCallable, Category = "Photography")
	static void SetIsPhotographyAllowed(const bool bIsPhotographyAllowed);

	/** Sets the number of frames between captures in a multi-part shot.  See CVar r.Photography.SettleFrames */
	//设置多部分镜头中捕获之间的帧数。 见 CVar r.Photography.SettleFrames
	UFUNCTION(BlueprintCallable, Category = "Photography")
	static void SetSettleFrames(const int NumSettleFrames);

	/** Sets the normal speed of movement of the photography camera.  See CVar r.Photography.TranslationSpeed */
	UFUNCTION(BlueprintCallable, Category = "Photography")
	static void SetCameraMovementSpeed(const float TranslationSpeed);

	/** Sets the size of the photography camera for collision purposes; only relevant when default implementation of PlayerCameraManager's PhotographyCameraModify function is used.  See CVar r.Photography.Constrain.CameraSize */
	UFUNCTION(BlueprintCallable, Category = "Photography")
	static void SetCameraConstraintCameraSize(const float CameraSize);

	/** Sets maximum distance which the camera is allowed to wander from its initial position; only relevant when default implementation of PlayerCameraManager's PhotographyCameraModify function is used.  See CVar r.Photography.Constrain.MaxCameraDistance */
	UFUNCTION(BlueprintCallable, Category = "Photography")
	static void SetCameraConstraintDistance(const float MaxCameraDistance);

	/** Sets whether the photography system automatically tries to optimize Unreal's postprocessing effects for photography.  See CVar r.Photography.AutoPostprocess */
	UFUNCTION(BlueprintCallable, Category = "Photography")
	static void SetAutoPostprocess(const bool bShouldAutoPostprocess);

	/** Sets whether the photography system automatically pauses the game during a photography session.  See CVar r.Photography.AutoPause */
	UFUNCTION(BlueprintCallable, Category = "Photography")
	static void SetAutoPause(const bool bShouldAutoPause);

	/** Show or hide controls in the photography UI which let the player tweak standard UE visual effects during photography - for example, depth of field or chromatic aberration.  Note: these controls only exist when SetAutoPostprocess is turned on.  Some may not apply to your application either because you are not using the associated effect or you are using a custom version of the effect. */
	// 在摄影 UI 中显示或隐藏控件，让玩家在摄影期间调整标准 UE 视觉效果 - 例如景深或色差。 注意：这些控件仅在 SetAutoPostprocess 打开时存在。 有些可能不适用于您的应用程序，因为您没有使用关联的效果，或者您使用的是自定义版本的效果。
	UFUNCTION(BlueprintCallable, Category = "Photography", meta = (WorldContext = WorldContextObject))
	static void SetUIControlVisibility(UObject* WorldContextObject, const TEnumAsByte<EUIControlEffectTarget> UIControlTarget, const bool bIsVisible);

	/** A utility which constrains distance of camera from its start point; may be useful when implementing a custom APlayerCameraManager::PhotographyCameraModify */
	UFUNCTION(BlueprintCallable, Category = "Photography", meta = (WorldContext = WorldContextObject))
	static void ConstrainCameraByDistance(UObject* WorldContextObject, const FVector NewCameraLocation, const FVector PreviousCameraLocation, const FVector OriginalCameraLocation, FVector& OutCameraLocation, float MaxDistance);

	/** A utility which constrains the camera against collidable geometry; may be useful when implementing a custom APlayerCameraManager::PhotographyCameraModify */
	UFUNCTION(BlueprintCallable, Category = "Photography", meta = (WorldContext = WorldContextObject))
	static void ConstrainCameraByGeometry(UObject* WorldContextObject, const FVector NewCameraLocation, const FVector PreviousCameraLocation, const FVector OriginalCameraLocation, FVector& OutCameraLocation);
};
