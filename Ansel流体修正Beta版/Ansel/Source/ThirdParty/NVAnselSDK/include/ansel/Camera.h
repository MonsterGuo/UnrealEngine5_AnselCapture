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
#include <nv/Vec3.h>
#include <nv/Quat.h>

//在ansel的命名空间里添加一个相机结构体
namespace ansel
{
    struct Camera
    {
        // Position of camera, in the game's coordinate space 
        // 记录相机所在位置
        nv::Vec3 position;
        // Rotation of the camera, in the game's coordinate space. I.e. if you apply this
        // rotation to the default orientation of the game's camera you will get the current
        // orientation of the camera (again, in game's coordinate space)
        // 一个用于矩阵换算的 四元（w（x,y,z））
        nv::Quat rotation;
        // Field of view in degrees. This value is either vertical or horizontal field of
        // view based on the 'fovType' setting passed in with setConfiguration.
        // 相机的Fov值
        float fov;
        // The amount that the projection matrix needs to be offset by. These values are
        // applied directly as translations to the projection matrix. These values are only
        // non-zero during Highres capture.
        // 相机矩阵的偏移函数
        float projectionOffsetX, projectionOffsetY;
        // Values of the near and far planes
        // 近处片，远处片
        float nearPlane, farPlane;
        // Projection matrix aspect ratio
        // 宽幅比
        float aspectRatio;
    };

    // Must be called on every frame an Ansel session is active. The 'camera' must contain
    // 必须的每一帧更新相机，相机必须包含
    // 当前显示的相机设置被调用时，后面调用又得重新请求一个新的相机
    // the current display camera settings when called. After calling 'camera' will contain the
    // new requested camera from Ansel.
    ANSEL_SDK_API void updateCamera(Camera& camera);

    // 转换一个思远得矩阵到旋转矩阵
    // Converts quaternion to rotation matrix vectors.
    ANSEL_SDK_API void quaternionToRotationMatrixVectors(const nv::Quat& q, nv::Vec3& right, nv::Vec3& up, nv::Vec3& forward);

    // 把一个旋转矩阵转换成四元矩阵
    // Converts rotation matrix vectors to quaternion. 
    ANSEL_SDK_API void rotationMatrixVectorsToQuaternion(const nv::Vec3& right, const nv::Vec3& up, const nv::Vec3& forward, nv::Quat& q);
}

