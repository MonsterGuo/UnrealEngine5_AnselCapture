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
#include <ansel/Session.h>
#include <ansel/Version.h>
#include <nv/Vec3.h>
#include <stdint.h>

namespace ansel
{
    //设置配置状态
    enum SetConfigurationStatus
    {
        // successfully initialized the Ansel SDK
        // 成功初始化Ansel SDK
        kSetConfigurationSuccess,
        // the version provided in the Configuration structure is not the same as the one stored inside the SDK binary (header/binary mismatch)
        // 配置结构中提供的版本与存储在 SDK 二进制文件中的版本不同（头文件/二进制文件不匹配） 
        kSetConfigurationIncompatibleVersion,
        // the Configuration structure supplied for the setConfiguration call is not consistent
        // 为 setConfiguration 调用提供的配置结构不一致
        kSetConfigurationIncorrectConfiguration,
        // the Ansel SDK is delay loaded and setConfiguration is called before the SDK is actually loaded
        // Ansel SDK 延迟加载并在 SDK 实际加载之前调用 setConfiguration
        kSetConfigurationSdkNotLoaded
    };
    //Fov的类型
    enum FovType
    {
        kHorizontalFov, //垂直FOV
        kVerticalFov    //水平FOV
    };
    //配置的结构体
    struct Configuration
    {
        // Basis vectors used by the game. They specify the handedness and orientation of 
        // the game's coordinate system. Think of them as the default orientation of the game
        // camera.
        //戏使用的基向量。 它们指定游戏坐标系的惯用手和方向。 将它们视为游戏摄像机的默认方向。
        // 设置相机的指向
        nv::Vec3 right, up, forward;
        // The speed at which camera moves in the world
        // 相机在世界中的移动速度
        float translationalSpeedInWorldUnitsPerSecond;
        // The speed at which camera rotates 
        // 相机的旋转速度
        float rotationalSpeedInDegreesPerSecond;
        // How many frames it takes for camera update to be reflected in a rendered frame
        // 相机更新需要多少帧才能反映在渲染帧中
        uint32_t captureLatency;
        // How many frames we must wait for a new frame to settle - i.e. temporal AA and similar
        // effects to stabilize after the camera has been adjusted
        //我们必须等待多少帧才能稳定新帧 - 即在调整相机后稳定时间 AA 和类似效果
        uint32_t captureSettleLatency;
        // Game scale, the size of a world unit measured in meters
        // 游戏的缩放 ，以米为单位的世界单位的大小
        float metersInWorldUnit;
        // Integration will support Camera::projectionOffsetX/projectionOffsetY
        // 集成将支持 Camera::projectionOffsetX/projectionOffsetY
        bool isCameraOffcenteredProjectionSupported;
        // Integration will support Camera::position
        // 集成将支持 相机：：位置
        bool isCameraTranslationSupported;
        // Integration will support Camera::rotation
        // 集成将支持 相机：：选装
        bool isCameraRotationSupported;
        // Integration will support Camera::horizontalFov
        // 集成将支持 相机：：水平FOV
        bool isCameraFovSupported;
        // 游戏名  是utf8编码 使用名字的结果来自图像文件的抓取
        // Game name, in utf8 encoding, used to name the resulting image files from capturing.
        // 不强制设置此字段。 选择的名称基于以下内容 
        // It is not mandatory to set this field. The name chosen is based on the following
        // 选定文件夹 
        // selection order:
        // 1. If GeForce profile exists for the game that name will be used
        // 如果 Geforce 存在 这个游戏将被使用
        // 2. If 'titleNameUtf8' is set that will be used
        // 如果 抬头名Utf8 将被使用
        // 3. The executable name is used as a last resort
        // 这个执行名是被用作最后的后缀
        const char* titleNameUtf8;
        // Camera structure will contain vertical FOV if this is set to kVerticalFov
        // 相机结构体 将被包含 垂直FOV 如果这个设置 垂直Fov
        // but horizontal FOV if this is set to kHorizontalFov. To simplify integration set
        // 但是水平Fov 如果 设置水平Fov ，简单的内置设置
        // this to the same orientation as the game is using.
        // 这些 相同旋转 作为游戏使用
        FovType fovType;

        // These callbacks will be called on the same thread Present()/glSwapBuffers is called
        // 这些回调将在调用 Present()/glSwapBuffers 的同一线程上调用
        // The thread calling to updateCamera() might be a different thread

        // User defined pointer which is then passed to all the callbacks (nullptr by default)
        // 调用 updateCamera() 的线程可能是不同的线程
        void* userPointer;

        // The window handle for the game/application where input messages are processed
        // 处理输入消息的游戏/应用程序的窗口句柄
        void* gameWindowHandle;

        // Called when user activates Ansel. Return kDisallowed if the game cannot comply with the
        // request. If the function returns kAllowed the following must be done:
        // 当用户激活 Ansel 时调用。 如果游戏不能满足请求，则返回 kDisallowed。 如果函数返回 kAllowed，则必须执行以下操作：
        // 1 更改 会话配置设置 但是 你仅仅需要去（这个对象已经设置为默认设置）
        // 1. Change the SessionConfigruation settings, but only where you need to (the object
        //    is already populated with default settings).
        // 2.在下一个更新循环中，游戏将处于 Ansel 会话中。 在 Ansel 会话期间，游戏
        //  a. 必须停止绘制UI和UHD元素在荧幕上，包含鼠标点击
        //  b. 必须调用Ansel：更新相机 每一帧
        //  c. 应该暂停渲染时间（例如没有移动的东仔将被在世界内可见）
        //  d.不应作用于鼠标和键盘的任何输入，也不得作用于游戏手柄的任何输入
        // 2. On the next update loop the game will be in an Ansel session. During an Ansel session 
        //    the game :
        //    a) Must stop drawing UI and HUD elements on the screen, including mouse cursor
        //    b) Must call ansel::updateCamera on every frame
        //    c) Should pause rendering time (i.e. no movement should be visible in the world)
        //    d) Should not act on any input from mouse and keyboard and must not act on any input 
        //       from gamepads
        // 3. Step 2 is repeated on every iteration of update loop until Session is stopped.
        // 3.在更新循环的每次迭代中重复第 2 步，直到 Session 停止。
        StartSessionCallback startSessionCallback;

        // Called when Ansel is deactivated. This call will only be made if the previous call
        // 停用 Ansel 时调用。 仅当上一次调用时才会进行此调用
        // to the startSessionCallback returned kAllowed.
        // 启动会话回调 将返回 kAllowed
        // Normally games will use this callback to restore their camera to the settings it had 
        // when the Ansel session was started.
        // 通常，游戏将使用此回调将其相机恢复到启动 Ansel 会话时的设置。
        StopSessionCallback stopSessionCallback;

        // 当抓取多部分的截图例如（高分辨率，360等等）已经开始
        // Called when the capture of a multipart shot (highres, 360, etc) has started.
        // 方便禁用那些不统一的全屏效果（如晕影）
        // Handy to disable those fullscreen effects that aren't uniform (like vignette)
        // 此回调是可选的（如果不需要，请保留 nullptr）
        // This callback is optional (leave nullptr if not needed)
        StartCaptureCallback startCaptureCallback;

        // 当抓取多部分的截图例如（高分辨率，360等等）已经停止
        // Called when the capture of a multipart shot (highres, 360, etc) has stopped.
        // 方便启用那些不统一的全屏效果那些被启动抓取关闭的回调函数
        // Handy to enable those fullscreen effects that were disabled by startCaptureCallback.
        // 此回调是可选的（如果不需要，请保留 nullptr）
        // This callback is optional (leave nullptr if not needed)
        StopCaptureCallback stopCaptureCallback;

        // The 'isFilterOutsideSessionAllowed' setting has been phased out in version 1.3 of the
        // SDK. This feature was only temporarily supported and no games took advantage of it.
        // 'isFilterOutsideSessionAllowed' 设置已在 SDK 1.3 版中逐步淘汰。 此功能只是暂时支持，没有游戏利用它。
        bool unused2;

        // The 'isExrSupported' setting has been phased out in version 1.1 of the SDK. Use
        // 'isRawAllowed' setting in SessionConfiguration to enable/disable captures into EXR
        // format.
        // 'isExrSupported' 设置已在 SDK 1.1 版中逐步淘汰。 
        //使用 SessionConfiguration 中的“isRawAllowed”设置启用/禁用捕获为 EXR 格式。
        bool unused1;

        // Holds the sdk version, doesn't require modifications
        // 保存 sdk 版本，不需要修改
        uint64_t sdkVersion;

        // Called when Ansel wants to give the application an opportunity to change image quality.
        // For example, keeping normal quality in-game for setting up a shot, but boosting quality
        // when actually taking the shot, then dropping quality back down to normal afterwards
        // we provide a binary normal/high quality switch using the bool argument (normal=false,high=true)
        // if the application wants to provide finer controls for quality, it can do so through 
        // general Ansel SDK user controls
        ChangeQualityCallback changeQualityCallback;

        Configuration()
        {
            right.x = 0.0f;
            right.y = 1.0f;
            right.z = 0.0f;
            up.x = 0.0f;
            up.y = 0.0f;
            up.z = 1.0f;
            forward.x = 1.0f;
            forward.y = 0.0f;
            forward.z = 0.0f;
            translationalSpeedInWorldUnitsPerSecond = 1.0f;
            rotationalSpeedInDegreesPerSecond = 45.0f;
            captureLatency = 1;
            captureSettleLatency = 0;
            metersInWorldUnit = 1.0f;
            isCameraOffcenteredProjectionSupported = true;
            isCameraTranslationSupported = true;
            isCameraRotationSupported = true;
            isCameraFovSupported = true;
            titleNameUtf8 = nullptr;
            fovType = kHorizontalFov;
            userPointer = nullptr;
            gameWindowHandle = 0;
            startSessionCallback = nullptr;
            stopSessionCallback = nullptr;
            startCaptureCallback = nullptr;
            stopCaptureCallback = nullptr;
            unused2 = true;
            unused1 = false;
            changeQualityCallback = nullptr;
            sdkVersion = ANSEL_SDK_VERSION;
        }
    };

    // Called during startup by the game. See 'Configuration' for further documentation.
    // 在游戏启动时调用。 有关更多文档，请参阅“配置”。
    ANSEL_SDK_API SetConfigurationStatus setConfiguration(const Configuration& cfg);

    // Can be called *after* D3D device has been created to see if Ansel is available.
    // 可以在创建 D3D 设备后*调用，以查看 Ansel 是否可用。
    // If called prior to D3D device creation it will always return false.
    // 如果在创建 D3D 设备之前调用它，它将始终返回 false。
    // Can be called *before* calling setConfiguration in which case it'll return true if Ansel is potentially available, otherwise false.
    // 可以在调用 setConfiguration 之前 * 调用，在这种情况下，如果 Ansel 可能可用，它将返回 true，否则返回 false。
    // If setConfiguration fails for any reason this function will return false (even if Ansel would be available otherwise)
    // 直到成功调用设置配置。
    // until a successfull call to setConfiguration has been made.
    ANSEL_SDK_API bool isAnselAvailable();
}

