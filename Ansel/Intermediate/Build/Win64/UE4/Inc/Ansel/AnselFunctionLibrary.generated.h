// Copyright Epic Games, Inc. All Rights Reserved.
/*===========================================================================
	Generated code exported from UnrealHeaderTool.
	DO NOT modify this manually! Edit the corresponding .h files instead!
===========================================================================*/

#include "UObject/ObjectMacros.h"
#include "UObject/ScriptMacros.h"

PRAGMA_DISABLE_DEPRECATION_WARNINGS
class UObject;
struct FVector;
#ifdef ANSEL_AnselFunctionLibrary_generated_h
#error "AnselFunctionLibrary.generated.h already included, missing '#pragma once' in AnselFunctionLibrary.h"
#endif
#define ANSEL_AnselFunctionLibrary_generated_h

#define Engine_Plugins_Runtime_Nvidia_Ansel_Source_Ansel_Public_AnselFunctionLibrary_h_20_SPARSE_DATA
#define Engine_Plugins_Runtime_Nvidia_Ansel_Source_Ansel_Public_AnselFunctionLibrary_h_20_RPC_WRAPPERS \
 \
	DECLARE_FUNCTION(execConstrainCameraByGeometry); \
	DECLARE_FUNCTION(execConstrainCameraByDistance); \
	DECLARE_FUNCTION(execSetUIControlVisibility); \
	DECLARE_FUNCTION(execSetAutoPause); \
	DECLARE_FUNCTION(execSetAutoPostprocess); \
	DECLARE_FUNCTION(execSetCameraConstraintDistance); \
	DECLARE_FUNCTION(execSetCameraConstraintCameraSize); \
	DECLARE_FUNCTION(execSetCameraMovementSpeed); \
	DECLARE_FUNCTION(execSetSettleFrames); \
	DECLARE_FUNCTION(execSetIsPhotographyAllowed); \
	DECLARE_FUNCTION(execIsPhotographyAllowed); \
	DECLARE_FUNCTION(execIsPhotographyAvailable); \
	DECLARE_FUNCTION(execStopSession); \
	DECLARE_FUNCTION(execStartSession);


#define Engine_Plugins_Runtime_Nvidia_Ansel_Source_Ansel_Public_AnselFunctionLibrary_h_20_RPC_WRAPPERS_NO_PURE_DECLS \
 \
	DECLARE_FUNCTION(execConstrainCameraByGeometry); \
	DECLARE_FUNCTION(execConstrainCameraByDistance); \
	DECLARE_FUNCTION(execSetUIControlVisibility); \
	DECLARE_FUNCTION(execSetAutoPause); \
	DECLARE_FUNCTION(execSetAutoPostprocess); \
	DECLARE_FUNCTION(execSetCameraConstraintDistance); \
	DECLARE_FUNCTION(execSetCameraConstraintCameraSize); \
	DECLARE_FUNCTION(execSetCameraMovementSpeed); \
	DECLARE_FUNCTION(execSetSettleFrames); \
	DECLARE_FUNCTION(execSetIsPhotographyAllowed); \
	DECLARE_FUNCTION(execIsPhotographyAllowed); \
	DECLARE_FUNCTION(execIsPhotographyAvailable); \
	DECLARE_FUNCTION(execStopSession); \
	DECLARE_FUNCTION(execStartSession);


#define Engine_Plugins_Runtime_Nvidia_Ansel_Source_Ansel_Public_AnselFunctionLibrary_h_20_INCLASS_NO_PURE_DECLS \
private: \
	static void StaticRegisterNativesUAnselFunctionLibrary(); \
	friend struct Z_Construct_UClass_UAnselFunctionLibrary_Statics; \
public: \
	DECLARE_CLASS(UAnselFunctionLibrary, UBlueprintFunctionLibrary, COMPILED_IN_FLAGS(0), CASTCLASS_None, TEXT("/Script/Ansel"), NO_API) \
	DECLARE_SERIALIZER(UAnselFunctionLibrary)


#define Engine_Plugins_Runtime_Nvidia_Ansel_Source_Ansel_Public_AnselFunctionLibrary_h_20_INCLASS \
private: \
	static void StaticRegisterNativesUAnselFunctionLibrary(); \
	friend struct Z_Construct_UClass_UAnselFunctionLibrary_Statics; \
public: \
	DECLARE_CLASS(UAnselFunctionLibrary, UBlueprintFunctionLibrary, COMPILED_IN_FLAGS(0), CASTCLASS_None, TEXT("/Script/Ansel"), NO_API) \
	DECLARE_SERIALIZER(UAnselFunctionLibrary)


#define Engine_Plugins_Runtime_Nvidia_Ansel_Source_Ansel_Public_AnselFunctionLibrary_h_20_STANDARD_CONSTRUCTORS \
	/** Standard constructor, called after all reflected properties have been initialized */ \
	NO_API UAnselFunctionLibrary(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get()); \
	DEFINE_DEFAULT_OBJECT_INITIALIZER_CONSTRUCTOR_CALL(UAnselFunctionLibrary) \
	DECLARE_VTABLE_PTR_HELPER_CTOR(NO_API, UAnselFunctionLibrary); \
	DEFINE_VTABLE_PTR_HELPER_CTOR_CALLER(UAnselFunctionLibrary); \
private: \
	/** Private move- and copy-constructors, should never be used */ \
	NO_API UAnselFunctionLibrary(UAnselFunctionLibrary&&); \
	NO_API UAnselFunctionLibrary(const UAnselFunctionLibrary&); \
public:


#define Engine_Plugins_Runtime_Nvidia_Ansel_Source_Ansel_Public_AnselFunctionLibrary_h_20_ENHANCED_CONSTRUCTORS \
	/** Standard constructor, called after all reflected properties have been initialized */ \
	NO_API UAnselFunctionLibrary(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get()) : Super(ObjectInitializer) { }; \
private: \
	/** Private move- and copy-constructors, should never be used */ \
	NO_API UAnselFunctionLibrary(UAnselFunctionLibrary&&); \
	NO_API UAnselFunctionLibrary(const UAnselFunctionLibrary&); \
public: \
	DECLARE_VTABLE_PTR_HELPER_CTOR(NO_API, UAnselFunctionLibrary); \
	DEFINE_VTABLE_PTR_HELPER_CTOR_CALLER(UAnselFunctionLibrary); \
	DEFINE_DEFAULT_OBJECT_INITIALIZER_CONSTRUCTOR_CALL(UAnselFunctionLibrary)


#define Engine_Plugins_Runtime_Nvidia_Ansel_Source_Ansel_Public_AnselFunctionLibrary_h_20_PRIVATE_PROPERTY_OFFSET
#define Engine_Plugins_Runtime_Nvidia_Ansel_Source_Ansel_Public_AnselFunctionLibrary_h_17_PROLOG
#define Engine_Plugins_Runtime_Nvidia_Ansel_Source_Ansel_Public_AnselFunctionLibrary_h_20_GENERATED_BODY_LEGACY \
PRAGMA_DISABLE_DEPRECATION_WARNINGS \
public: \
	Engine_Plugins_Runtime_Nvidia_Ansel_Source_Ansel_Public_AnselFunctionLibrary_h_20_PRIVATE_PROPERTY_OFFSET \
	Engine_Plugins_Runtime_Nvidia_Ansel_Source_Ansel_Public_AnselFunctionLibrary_h_20_SPARSE_DATA \
	Engine_Plugins_Runtime_Nvidia_Ansel_Source_Ansel_Public_AnselFunctionLibrary_h_20_RPC_WRAPPERS \
	Engine_Plugins_Runtime_Nvidia_Ansel_Source_Ansel_Public_AnselFunctionLibrary_h_20_INCLASS \
	Engine_Plugins_Runtime_Nvidia_Ansel_Source_Ansel_Public_AnselFunctionLibrary_h_20_STANDARD_CONSTRUCTORS \
public: \
PRAGMA_ENABLE_DEPRECATION_WARNINGS


#define Engine_Plugins_Runtime_Nvidia_Ansel_Source_Ansel_Public_AnselFunctionLibrary_h_20_GENERATED_BODY \
PRAGMA_DISABLE_DEPRECATION_WARNINGS \
public: \
	Engine_Plugins_Runtime_Nvidia_Ansel_Source_Ansel_Public_AnselFunctionLibrary_h_20_PRIVATE_PROPERTY_OFFSET \
	Engine_Plugins_Runtime_Nvidia_Ansel_Source_Ansel_Public_AnselFunctionLibrary_h_20_SPARSE_DATA \
	Engine_Plugins_Runtime_Nvidia_Ansel_Source_Ansel_Public_AnselFunctionLibrary_h_20_RPC_WRAPPERS_NO_PURE_DECLS \
	Engine_Plugins_Runtime_Nvidia_Ansel_Source_Ansel_Public_AnselFunctionLibrary_h_20_INCLASS_NO_PURE_DECLS \
	Engine_Plugins_Runtime_Nvidia_Ansel_Source_Ansel_Public_AnselFunctionLibrary_h_20_ENHANCED_CONSTRUCTORS \
private: \
PRAGMA_ENABLE_DEPRECATION_WARNINGS


template<> ANSEL_API UClass* StaticClass<class UAnselFunctionLibrary>();

#undef CURRENT_FILE_ID
#define CURRENT_FILE_ID Engine_Plugins_Runtime_Nvidia_Ansel_Source_Ansel_Public_AnselFunctionLibrary_h


#define FOREACH_ENUM_EUICONTROLEFFECTTARGET(op) \
	op(Bloom) \
	op(DepthOfField) \
	op(ChromaticAberration) \
	op(MotionBlur) 
PRAGMA_ENABLE_DEPRECATION_WARNINGS
