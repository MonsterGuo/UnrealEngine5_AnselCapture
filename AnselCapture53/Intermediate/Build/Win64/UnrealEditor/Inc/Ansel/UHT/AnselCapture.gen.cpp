// Copyright Epic Games, Inc. All Rights Reserved.
/*===========================================================================
	Generated code exported from UnrealHeaderTool.
	DO NOT modify this manually! Edit the corresponding .h files instead!
===========================================================================*/

#include "UObject/GeneratedCppIncludes.h"
#include "Ansel/Public/AnselCapture.h"
PRAGMA_DISABLE_DEPRECATION_WARNINGS
void EmptyLinkFunctionForGeneratedCodeAnselCapture() {}
// Cross Module References
	ANSEL_API UClass* Z_Construct_UClass_AAnselCapture();
	ANSEL_API UClass* Z_Construct_UClass_AAnselCapture_NoRegister();
	ENGINE_API UClass* Z_Construct_UClass_AActor();
	UPackage* Z_Construct_UPackage__Script_Ansel();
// End Cross Module References
	DEFINE_FUNCTION(AAnselCapture::execForceQualitySettings)
	{
		P_FINISH;
		P_NATIVE_BEGIN;
		P_THIS->ForceQualitySettings();
		P_NATIVE_END;
	}
	DEFINE_FUNCTION(AAnselCapture::execCallSequncePlayer)
	{
		P_FINISH;
		P_NATIVE_BEGIN;
		P_THIS->CallSequncePlayer_Implementation();
		P_NATIVE_END;
	}
	static FName NAME_AAnselCapture_CallSequncePlayer = FName(TEXT("CallSequncePlayer"));
	void AAnselCapture::CallSequncePlayer()
	{
		ProcessEvent(FindFunctionChecked(NAME_AAnselCapture_CallSequncePlayer),NULL);
	}
	void AAnselCapture::StaticRegisterNativesAAnselCapture()
	{
		UClass* Class = AAnselCapture::StaticClass();
		static const FNameNativePtrPair Funcs[] = {
			{ "CallSequncePlayer", &AAnselCapture::execCallSequncePlayer },
			{ "ForceQualitySettings", &AAnselCapture::execForceQualitySettings },
		};
		FNativeFunctionRegistrar::RegisterFunctions(Class, Funcs, UE_ARRAY_COUNT(Funcs));
	}
	struct Z_Construct_UFunction_AAnselCapture_CallSequncePlayer_Statics
	{
#if WITH_METADATA
		static const UECodeGen_Private::FMetaDataPairParam Function_MetaDataParams[];
#endif
		static const UECodeGen_Private::FFunctionParams FuncParams;
	};
#if WITH_METADATA
	const UECodeGen_Private::FMetaDataPairParam Z_Construct_UFunction_AAnselCapture_CallSequncePlayer_Statics::Function_MetaDataParams[] = {
		{ "Category", "Photography" },
		{ "ModuleRelativePath", "Public/AnselCapture.h" },
	};
#endif
	const UECodeGen_Private::FFunctionParams Z_Construct_UFunction_AAnselCapture_CallSequncePlayer_Statics::FuncParams = { (UObject*(*)())Z_Construct_UClass_AAnselCapture, nullptr, "CallSequncePlayer", nullptr, nullptr, nullptr, 0, 0, RF_Public|RF_Transient|RF_MarkAsNative, (EFunctionFlags)0x08020C00, 0, 0, METADATA_PARAMS(UE_ARRAY_COUNT(Z_Construct_UFunction_AAnselCapture_CallSequncePlayer_Statics::Function_MetaDataParams), Z_Construct_UFunction_AAnselCapture_CallSequncePlayer_Statics::Function_MetaDataParams) };
	UFunction* Z_Construct_UFunction_AAnselCapture_CallSequncePlayer()
	{
		static UFunction* ReturnFunction = nullptr;
		if (!ReturnFunction)
		{
			UECodeGen_Private::ConstructUFunction(&ReturnFunction, Z_Construct_UFunction_AAnselCapture_CallSequncePlayer_Statics::FuncParams);
		}
		return ReturnFunction;
	}
	struct Z_Construct_UFunction_AAnselCapture_ForceQualitySettings_Statics
	{
#if WITH_METADATA
		static const UECodeGen_Private::FMetaDataPairParam Function_MetaDataParams[];
#endif
		static const UECodeGen_Private::FFunctionParams FuncParams;
	};
#if WITH_METADATA
	const UECodeGen_Private::FMetaDataPairParam Z_Construct_UFunction_AAnselCapture_ForceQualitySettings_Statics::Function_MetaDataParams[] = {
		{ "Category", "Photography" },
		{ "ModuleRelativePath", "Public/AnselCapture.h" },
	};
#endif
	const UECodeGen_Private::FFunctionParams Z_Construct_UFunction_AAnselCapture_ForceQualitySettings_Statics::FuncParams = { (UObject*(*)())Z_Construct_UClass_AAnselCapture, nullptr, "ForceQualitySettings", nullptr, nullptr, nullptr, 0, 0, RF_Public|RF_Transient|RF_MarkAsNative, (EFunctionFlags)0x04020401, 0, 0, METADATA_PARAMS(UE_ARRAY_COUNT(Z_Construct_UFunction_AAnselCapture_ForceQualitySettings_Statics::Function_MetaDataParams), Z_Construct_UFunction_AAnselCapture_ForceQualitySettings_Statics::Function_MetaDataParams) };
	UFunction* Z_Construct_UFunction_AAnselCapture_ForceQualitySettings()
	{
		static UFunction* ReturnFunction = nullptr;
		if (!ReturnFunction)
		{
			UECodeGen_Private::ConstructUFunction(&ReturnFunction, Z_Construct_UFunction_AAnselCapture_ForceQualitySettings_Statics::FuncParams);
		}
		return ReturnFunction;
	}
	IMPLEMENT_CLASS_NO_AUTO_REGISTRATION(AAnselCapture);
	UClass* Z_Construct_UClass_AAnselCapture_NoRegister()
	{
		return AAnselCapture::StaticClass();
	}
	struct Z_Construct_UClass_AAnselCapture_Statics
	{
		static UObject* (*const DependentSingletons[])();
		static const FClassFunctionLinkInfo FuncInfo[];
#if WITH_METADATA
		static const UECodeGen_Private::FMetaDataPairParam Class_MetaDataParams[];
#endif
		static const FCppClassTypeInfoStatic StaticCppClassTypeInfo;
		static const UECodeGen_Private::FClassParams ClassParams;
	};
	UObject* (*const Z_Construct_UClass_AAnselCapture_Statics::DependentSingletons[])() = {
		(UObject* (*)())Z_Construct_UClass_AActor,
		(UObject* (*)())Z_Construct_UPackage__Script_Ansel,
	};
	static_assert(UE_ARRAY_COUNT(Z_Construct_UClass_AAnselCapture_Statics::DependentSingletons) < 16);
	const FClassFunctionLinkInfo Z_Construct_UClass_AAnselCapture_Statics::FuncInfo[] = {
		{ &Z_Construct_UFunction_AAnselCapture_CallSequncePlayer, "CallSequncePlayer" }, // 1694322907
		{ &Z_Construct_UFunction_AAnselCapture_ForceQualitySettings, "ForceQualitySettings" }, // 760362284
	};
	static_assert(UE_ARRAY_COUNT(Z_Construct_UClass_AAnselCapture_Statics::FuncInfo) < 2048);
#if WITH_METADATA
	const UECodeGen_Private::FMetaDataPairParam Z_Construct_UClass_AAnselCapture_Statics::Class_MetaDataParams[] = {
		{ "IncludePath", "AnselCapture.h" },
		{ "ModuleRelativePath", "Public/AnselCapture.h" },
	};
#endif
	const FCppClassTypeInfoStatic Z_Construct_UClass_AAnselCapture_Statics::StaticCppClassTypeInfo = {
		TCppClassTypeTraits<AAnselCapture>::IsAbstract,
	};
	const UECodeGen_Private::FClassParams Z_Construct_UClass_AAnselCapture_Statics::ClassParams = {
		&AAnselCapture::StaticClass,
		"Engine",
		&StaticCppClassTypeInfo,
		DependentSingletons,
		FuncInfo,
		nullptr,
		nullptr,
		UE_ARRAY_COUNT(DependentSingletons),
		UE_ARRAY_COUNT(FuncInfo),
		0,
		0,
		0x008000A4u,
		METADATA_PARAMS(UE_ARRAY_COUNT(Z_Construct_UClass_AAnselCapture_Statics::Class_MetaDataParams), Z_Construct_UClass_AAnselCapture_Statics::Class_MetaDataParams)
	};
	UClass* Z_Construct_UClass_AAnselCapture()
	{
		if (!Z_Registration_Info_UClass_AAnselCapture.OuterSingleton)
		{
			UECodeGen_Private::ConstructUClass(Z_Registration_Info_UClass_AAnselCapture.OuterSingleton, Z_Construct_UClass_AAnselCapture_Statics::ClassParams);
		}
		return Z_Registration_Info_UClass_AAnselCapture.OuterSingleton;
	}
	template<> ANSEL_API UClass* StaticClass<AAnselCapture>()
	{
		return AAnselCapture::StaticClass();
	}
	DEFINE_VTABLE_PTR_HELPER_CTOR(AAnselCapture);
	AAnselCapture::~AAnselCapture() {}
	struct Z_CompiledInDeferFile_FID_HostProject_Plugins_Ansel_Source_Ansel_Public_AnselCapture_h_Statics
	{
		static const FClassRegisterCompiledInInfo ClassInfo[];
	};
	const FClassRegisterCompiledInInfo Z_CompiledInDeferFile_FID_HostProject_Plugins_Ansel_Source_Ansel_Public_AnselCapture_h_Statics::ClassInfo[] = {
		{ Z_Construct_UClass_AAnselCapture, AAnselCapture::StaticClass, TEXT("AAnselCapture"), &Z_Registration_Info_UClass_AAnselCapture, CONSTRUCT_RELOAD_VERSION_INFO(FClassReloadVersionInfo, sizeof(AAnselCapture), 2915178606U) },
	};
	static FRegisterCompiledInInfo Z_CompiledInDeferFile_FID_HostProject_Plugins_Ansel_Source_Ansel_Public_AnselCapture_h_694163715(TEXT("/Script/Ansel"),
		Z_CompiledInDeferFile_FID_HostProject_Plugins_Ansel_Source_Ansel_Public_AnselCapture_h_Statics::ClassInfo, UE_ARRAY_COUNT(Z_CompiledInDeferFile_FID_HostProject_Plugins_Ansel_Source_Ansel_Public_AnselCapture_h_Statics::ClassInfo),
		nullptr, 0,
		nullptr, 0);
PRAGMA_ENABLE_DEPRECATION_WARNINGS
