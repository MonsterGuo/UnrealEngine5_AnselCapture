// This code contains NVIDIA Confidential Information and is disclosed to you
// under a form of NVIDIA software license agreement provided separately to you.
//
// Notice
// NVIDIA Corporation and its licensors retain all intellectual property and
// proprietary rights in and to this software and related documentation and
// any modifications thereto. Any use, reproduction, disclosure, or
// distribution of this software and related documentation without an express
// license agreement from NVIDIA Corporation is strictly prohibited.
//
// ALL NVIDIA DESIGN SPECIFICATIONS, CODE ARE PROVIDED "AS IS.". NVIDIA MAKES
// NO WARRANTIES, EXPRESSED, IMPLIED, STATUTORY, OR OTHERWISE WITH RESPECT TO
// THE MATERIALS, AND EXPRESSLY DISCLAIMS ALL IMPLIED WARRANTIES OF NONINFRINGEMENT,
// MERCHANTABILITY, AND FITNESS FOR A PARTICULAR PURPOSE.
//
// Information and code furnished is believed to be accurate and reliable.
// However, NVIDIA Corporation assumes no responsibility for the consequences of use of such
// information or for any infringement of patents or other rights of third parties that may
// result from its use. No license is granted by implication or otherwise under any patent
// or patent rights of NVIDIA Corporation. Details are subject to change without notice.
// This code supersedes and replaces all information previously supplied.
// NVIDIA Corporation products are not authorized for use as critical
// components in life support devices or systems without express written approval of
// NVIDIA Corporation.
//
// Copyright 2016 NVIDIA Corporation. All rights reserved.

#pragma once
#include <ansel/Defines.h>

namespace ansel
{
    //会话配置
    struct SessionConfiguration
    {
        // User can move the camera during session
        // 用户能移动相机，在节选之后
        bool isTranslationAllowed;
        // Camera can be rotated during session
        // 相机能不能旋转 在会话之后
        bool isRotationAllowed;
        // Fov 能被定义 在会话之后
        // FoV can be modified during session
        bool isFovChangeAllowed;
        // 游戏被暂停在抓取之后
        // Game is paused during capture
        bool isPauseAllowed;
        // 游戏允许高分辨率截图在会话之后
        // Game allows highres capture during session
        bool isHighresAllowed;
        // 游戏允许360 抓取 会话之后
        // Game allows 360 capture during session
        bool is360MonoAllowed;
        // Game allows 360 stereo capture during session
        // 游戏 360 立体抓取 会话之后
        bool is360StereoAllowed;
        // Game allows capturing pre-tonemapping raw HDR buffer
        // 游戏允许捕获预色调映射原始 HDR 缓冲区
        bool isRawAllowed;
        // The speed at which camera moves in the world, initialized with a value given in Configuration
        // 在世界中每秒移动速度单位
        float translationalSpeedInWorldUnitsPerSecond;
        // The speed at which camera rotates, initialized with a value given in Configuration
        // 每秒的旋转角度
        float rotationalSpeedInDegreesPerSecond;
        // The maximum FoV value in degrees displayed in the Ansel UI.
        // Any value in the range [140, 179] can be specified and values outside will be clamped to this range.
        // 最大Fov
        float maximumFovInDegrees;
        

        //构造函数
        SessionConfiguration()
        {
            isTranslationAllowed = true;
            isRotationAllowed = true;
            isFovChangeAllowed = true;
            isPauseAllowed = true;
            isHighresAllowed = true;
            is360MonoAllowed = true;
            is360StereoAllowed = true;
            isRawAllowed = false;
            maximumFovInDegrees = 140.0f;
        }
    };
    // 起始会话的状态枚举
    enum StartSessionStatus
    {
        kDisallowed = 0,
        kAllowed
    };
    //抓取类型的 枚举
    enum CaptureType
    {
        kCaptureType360Mono = 0,
        kCaptureType360Stereo,
        kCaptureTypeSuperResolution,
        kCaptureTypeStereo
    };
    // 抓取配置
    struct CaptureConfiguration
    {
        //抓取类别对象  
        CaptureType captureType;
    };

    typedef StartSessionStatus(__cdecl *StartSessionCallback)(SessionConfiguration& settings, void* userPointer);
    typedef void(__cdecl *StopSessionCallback)(void* userPointer);
    typedef void(__cdecl *StartCaptureCallback)(const CaptureConfiguration&, void* userPointer);
    typedef void(__cdecl *StopCaptureCallback)(void* userPointer);
    typedef void(__cdecl *ChangeQualityCallback)(bool isHighQuality, void* userPointer);

    // Starts a session if there is not one already active. This function can be used to trigger
    // Ansel via any method that the game chooses (key combination, controller input, etc)
    ANSEL_SDK_API void startSession();

    // Stops current session if there is one active. This function needs to be used if the
    // startSession function was used to implement custom activation method. The same custom
    // activation method needs to used to deactivate Ansel. If for instance left-stick pressed
    // was used for startSession then the same should be used for stopping the Ansel session.
    // This function can also be used if Ansel session needs to be interrupted
    // (for instance an online game where connection loss is experienced and needs to be
    // reported).
    ANSEL_SDK_API void stopSession();
}
