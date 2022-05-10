// Copyright Epic Games, Inc. All Rights Reserved.
/*===========================================================================
	Generated code exported from UnrealHeaderTool.
	DO NOT modify this manually! Edit the corresponding .h files instead!
===========================================================================*/

#include "UObject/GeneratedCppIncludes.h"
#include "Ansel/Public/AnselCapture.h"
#ifdef _MSC_VER
#pragma warning (push)
#pragma warning (disable : 4883)
#endif
PRAGMA_DISABLE_DEPRECATION_WARNINGS
void EmptyLinkFunctionForGeneratedCodeAnselCapture() {}
// Cross Module References
	ANSEL_API UClass* Z_Construct_UClass_AAnselCapture_NoRegister();
	ANSEL_API UClass* Z_Construct_UClass_AAnselCapture();
	ENGINE_API UClass* Z_Construct_UClass_AActor();
	UPackage* Z_Construct_UPackage__Script_Ansel();
// End Cross Module References
	DEFINE_FUNCTION(AAnselCapture::execTestCall)
	{
		P_FINISH;
		P_NATIVE_BEGIN;
		P_THIS->TestCall();
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
			{ "TestCall", &AAnselCapture::execTestCall },
		};
		FNativeFunctionRegistrar::RegisterFunctions(Class, Funcs, UE_ARRAY_COUNT(Funcs));
	}
	struct Z_Construct_UFunction_AAnselCapture_CallSequncePlayer_Statics
	{
#if WITH_METADATA
		static const UE4CodeGen_Private::FMetaDataPairParam Function_MetaDataParams[];
#endif
		static const UE4CodeGen_Private::FFunctionParams FuncParams;
	};
#if WITH_METADATA
	const UE4CodeGen_Private::FMetaDataPairParam Z_Construct_UFunction_AAnselCapture_CallSequncePlayer_Statics::Function_MetaDataParams[] = {
		{ "Category", "Photography" },
		{ "ModuleRelativePath", "Public/AnselCapture.h" },
	};
#endif
	const UE4CodeGen_Private::FFunctionParams Z_Construct_UFunction_AAnselCapture_CallSequncePlayer_Statics::FuncParams = { (UObject*(*)())Z_Construct_UClass_AAnselCapture, nullptr, "CallSequncePlayer", nullptr, nullptr, 0, nullptr, 0, RF_Public|RF_Transient|RF_MarkAsNative, (EFunctionFlags)0x08020C00, 0, 0, METADATA_PARAMS(Z_Construct_UFunction_AAnselCapture_CallSequncePlayer_Statics::Function_MetaDataParams, UE_ARRAY_COUNT(Z_Construct_UFunction_AAnselCapture_CallSequncePlayer_Statics::Function_MetaDataParams)) };
	UFunction* Z_Construct_UFunction_AAnselCapture_CallSequncePlayer()
	{
		static UFunction* ReturnFunction = nullptr;
		if (!ReturnFunction)
		{
			UE4CodeGen_Private::ConstructUFunction(ReturnFunction, Z_Construct_UFunction_AAnselCapture_CallSequncePlayer_Statics::FuncParams);
		}
		return ReturnFunction;
	}
	struct Z_Construct_UFunction_AAnselCapture_TestCall_Statics
	{
#if WITH_METADATA
		static const UE4CodeGen_Private::FMetaDataPairParam Function_MetaDataParams[];
#endif
		static const UE4CodeGen_Private::FFunctionParams FuncParams;
	};
#if WITH_METADATA
	const UE4CodeGen_Private::FMetaDataPairParam Z_Construct_UFunction_AAnselCapture_TestCall_Statics::Function_MetaDataParams[] = {
		{ "Category", "Photography" },
		{ "ModuleRelativePath", "Public/AnselCapture.h" },
	};
#endif
	const UE4CodeGen_Private::FFunctionParams Z_Construct_UFunction_AAnselCapture_TestCall_Statics::FuncParams = { (UObject*(*)())Z_Construct_UClass_AAnselCapture, nullptr, "TestCall", nullptr, nullptr, 0, nullptr, 0, RF_Public|RF_Transient|RF_MarkAsNative, (EFunctionFlags)0x04020401, 0, 0, METADATA_PARAMS(Z_Construct_UFunction_AAnselCapture_TestCall_Statics::Function_MetaDataParams, UE_ARRAY_COUNT(Z_Construct_UFunction_AAnselCapture_TestCall_Statics::Function_MetaDataParams)) };
	UFunction* Z_Construct_UFunction_AAnselCapture_TestCall()
	{
		static UFunction* ReturnFunction = nullptr;
		if (!ReturnFunction)
		{
			UE4CodeGen_Private::ConstructUFunction(ReturnFunction, Z_Construct_UFunction_AAnselCapture_TestCall_Statics::FuncParams);
		}
		return ReturnFunction;
	}
	UClass* Z_Construct_UClass_AAnselCapture_NoRegister()
	{
		return AAnselCapture::StaticClass();
	}
	struct Z_Construct_UClass_AAnselCapture_Statics
	{
		static UObject* (*const DependentSingletons[])();
		static const FClassFunctionLinkInfo FuncInfo[];
#if WITH_METADATA
		static const UE4CodeGen_Private::FMetaDataPairParam Class_MetaDataParams[];
#endif
		static const FCppClassTypeInfoStatic StaticCppClassTypeInfo;
		static const UE4CodeGen_Private::FClassParams ClassParams;
	};
	UObject* (*const Z_Construct_UClass_AAnselCapture_Statics::DependentSingletons[])() = {
		(UObject* (*)())Z_Construct_UClass_AActor,
		(UObject* (*)())Z_Construct_UPackage__Script_Ansel,
	};
	const FClassFunctionLinkInfo Z_Construct_UClass_AAnselCapture_Statics::FuncInfo[] = {
		{ &Z_Construct_UFunction_AAnselCapture_CallSequncePlayer, "CallSequncePlayer" }, // 3612670973
		{ &Z_Construct_UFunction_AAnselCapture_TestCall, "TestCall" }, // 3710061124
	};
#if WITH_METADATA
	const UE4CodeGen_Private::FMetaDataPairParam Z_Construct_UClass_AAnselCapture_Statics::Class_MetaDataParams[] = {
		{ "IncludePath", "AnselCapture.h" },
		{ "ModuleRelativePath", "Public/AnselCapture.h" },
	};
#endif
	const FCppClassTypeInfoStatic Z_Construct_UClass_AAnselCapture_Statics::StaticCppClassTypeInfo = {
		TCppClassTypeTraits<AAnselCapture>::IsAbstract,
	};
	const UE4CodeGen_Private::FClassParams Z_Construct_UClass_AAnselCapture_Statics::ClassParams = {
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
		METADATA_PARAMS(Z_Construct_UClass_AAnselCapture_Statics::Class_MetaDataParams, UE_ARRAY_COUNT(Z_Construct_UClass_AAnselCapture_Statics::Class_MetaDataParams))
	};
	UClass* Z_Construct_UClass_AAnselCapture()
	{
		static UClass* OuterClass = nullptr;
		if (!OuterClass)
		{
			UE4CodeGen_Private::ConstructUClass(OuterClass, Z_Construct_UClass_AAnselCapture_Statics::ClassParams);
		}
		return OuterClass;
	}
	IMPLEMENT_CLASS(AAnselCapture, 3148112747);
	template<> ANSEL_API UClass* StaticClass<AAnselCapture>()
	{
		return AAnselCapture::StaticClass();
	}
	static FCompiledInDefer Z_CompiledInDefer_UClass_AAnselCapture(Z_Construct_UClass_AAnselCapture, &AAnselCapture::StaticClass, TEXT("/Script/Ansel"), TEXT("AAnselCapture"), false, nullptr, nullptr, nullptr);
	DEFINE_VTABLE_PTR_HELPER_CTOR(AAnselCapture);
PRAGMA_ENABLE_DEPRECATION_WARNINGS
#ifdef _MSC_VER
#pragma warning (pop)
#endif
