#include "AnselCapture.h"

// Sets default values
AAnselCapture::AAnselCapture()
{
	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;

}


void AAnselCapture::CallSequncePlayer_Implementation()
{
}


void AAnselCapture::ForceQualitySettings()
{
	GEngine->Exec(GetWorld(), TEXT("r.ScreenPercentage 100"));
	GEngine->Exec(GetWorld(), TEXT("r.gbufferformat 5"));
	GEngine->Exec(GetWorld(), TEXT("r.StaticMeshLODDistanceScale 0.25"));
	GEngine->Exec(GetWorld(), TEXT("r.skeletalmeshlodbias -10"));
	GEngine->Exec(GetWorld(), TEXT("r.TextureStreaming 0"));
	GEngine->Exec(GetWorld(), TEXT("r.ForceLOD 0"));
	GEngine->Exec(GetWorld(), TEXT("foliage.DitheredLOD 0"));
	GEngine->Exec(GetWorld(), TEXT("foliage.ForceLOD 0"));
	GEngine->Exec(GetWorld(), TEXT("r.HLOD"));
	GEngine->Exec(GetWorld(), TEXT("r.D3D12.GPUTimeout 0"));
	GEngine->Exec(GetWorld(), TEXT("a.URO.Enable 0"));
	GEngine->Exec(GetWorld(), TEXT("r.SkyLight.RealTimeReflectionCapture.TimeSlice 0"));
	GEngine->Exec(GetWorld(), TEXT("r.VolumetricRenderTarget 0"));
	GEngine->Exec(GetWorld(), TEXT("r.AntiAliasingMethod 2"));
	GEngine->Exec(GetWorld(), TEXT("r.TemporalAAFilterSize 1"));
	GEngine->Exec(GetWorld(), TEXT("r.TranslucencyLightingVolumeDim 64"));
	GEngine->Exec(GetWorld(), TEXT("r.RefractionQuality 2"));
	GEngine->Exec(GetWorld(), TEXT("r.SSR.Quality 4"));
	GEngine->Exec(GetWorld(), TEXT("r.TranslucencyVolumeBlur 1"));
	GEngine->Exec(GetWorld(), TEXT("r.MaterialQualityLevel 1"));
	GEngine->Exec(GetWorld(), TEXT("r.SSS.Scale 1"));
	GEngine->Exec(GetWorld(), TEXT("r.SSS.SampleSet 2"));
	GEngine->Exec(GetWorld(), TEXT("r.SSS.Quality 1"));
	GEngine->Exec(GetWorld(), TEXT("r.SSS.HalfRes 0"));
	GEngine->Exec(GetWorld(), TEXT("r.ParticleLightQuality 2"));
	GEngine->Exec(GetWorld(), TEXT("r.AmbientOcclusionMipLevelFactor 0.4"));
	GEngine->Exec(GetWorld(), TEXT("r.AmbientOcclusionMaxQuality 100"));
	GEngine->Exec(GetWorld(), TEXT("r.AmbientOcclusionLevels -1"));
	GEngine->Exec(GetWorld(), TEXT("r.AmbientOcclusionRadiusScale 1"));
	GEngine->Exec(GetWorld(), TEXT("r.DepthOfFieldQuality 4"));
	GEngine->Exec(GetWorld(), TEXT("r.RenderTargetPoolMin 500"));
	GEngine->Exec(GetWorld(), TEXT("r.LensFlareQuality 3"));
	GEngine->Exec(GetWorld(), TEXT("r.SceneColorFringeQuality 1"));
	GEngine->Exec(GetWorld(), TEXT("r.BloomQuality 5"));
	GEngine->Exec(GetWorld(), TEXT("r.FastBlurThreshold 100"));
	GEngine->Exec(GetWorld(), TEXT("r.Upscale.Quality 3"));
	GEngine->Exec(GetWorld(), TEXT("r.Tonemapper.GrainQuantization 1"));
	GEngine->Exec(GetWorld(), TEXT("r.LightShaftQuality 1"));
	GEngine->Exec(GetWorld(), TEXT("r.Filter.SizeScale 1"));
	GEngine->Exec(GetWorld(), TEXT("r.Tonemapper.Quality 5"));
	GEngine->Exec(GetWorld(), TEXT("r.DOF.Gather.AccumulatorQuality 1"));
	GEngine->Exec(GetWorld(), TEXT("r.DOF.Gather.PostfilterMethod 1"));
	GEngine->Exec(GetWorld(), TEXT("r.DOF.Gather.EnableBokehSettings 1"));
	GEngine->Exec(GetWorld(), TEXT("r.DOF.Gather.RingCount 5"));
	GEngine->Exec(GetWorld(), TEXT("r.DOF.Scatter.ForegroundCompositing 1"));
	GEngine->Exec(GetWorld(), TEXT("r.DOF.Scatter.BackgroundCompositing 2"));
	GEngine->Exec(GetWorld(), TEXT("r.DOF.Scatter.EnableBokehSettings 1"));
	GEngine->Exec(GetWorld(), TEXT("r.DOF.Scatter.MaxSpriteRatio 0.1"));
	GEngine->Exec(GetWorld(), TEXT("r.DOF.Recombine.Quality 2"));
	GEngine->Exec(GetWorld(), TEXT("r.DOF.Recombine.EnableBokehSettings 1"));
	GEngine->Exec(GetWorld(), TEXT("r.DOF.TemporalAAQuality 1"));
	GEngine->Exec(GetWorld(), TEXT("r.DOF.Kernel.MaxForegroundRadius 0.025"));
	GEngine->Exec(GetWorld(), TEXT("r.DOF.Kernel.MaxBackgroundRadius 0.025"));
	GEngine->Exec(GetWorld(), TEXT("r.Streaming.MipBias 8"));
	GEngine->Exec(GetWorld(), TEXT("r.MaxAnisotropy 16"));
	GEngine->Exec(GetWorld(), TEXT("r.Streaming.MaxEffectiveScreenSize 0"));
	GEngine->Exec(GetWorld(), TEXT("foliage.DensityScale 1"));
	GEngine->Exec(GetWorld(), TEXT("grass.DensityScale 1"));
	GEngine->Exec(GetWorld(), TEXT("foliage.LODDistanceScale 4"));
	GEngine->Exec(GetWorld(), TEXT("r.ViewDistanceScale 50"));
	GEngine->Exec(GetWorld(), TEXT("r.LightFunctionQuality 2"));
	GEngine->Exec(GetWorld(), TEXT("r.ShadowQuality 5"));
	GEngine->Exec(GetWorld(), TEXT("r.Shadow.CSM.MaxCascades 10"));
	GEngine->Exec(GetWorld(), TEXT("r.Shadow.MaxResolution 4096"));
	GEngine->Exec(GetWorld(), TEXT("r.Shadow.MaxCSMResolution 4096"));
	GEngine->Exec(GetWorld(), TEXT("r.Shadow.RadiusThreshold 0.001"));
	GEngine->Exec(GetWorld(), TEXT("r.Shadow.DistanceScale 10"));
	GEngine->Exec(GetWorld(), TEXT("r.Shadow.CSM.TransitionScale 1"));
	GEngine->Exec(GetWorld(), TEXT("r.Shadow.PreShadowResolutionFactor 1"));
	GEngine->Exec(GetWorld(), TEXT("r.AOQuality 2"));
	GEngine->Exec(GetWorld(), TEXT("r.VolumetricFog 1"));
	GEngine->Exec(GetWorld(), TEXT("r.VolumetricFog.GridPixelSize 4"));
	GEngine->Exec(GetWorld(), TEXT("r.VolumetricFog.GridSizeZ 128"));
	GEngine->Exec(GetWorld(), TEXT("r.VolumetricFog.HistoryMissSupersampleCount 16"));
	GEngine->Exec(GetWorld(), TEXT("r.LightMaxDrawDistanceScale 4"));
	GEngine->Exec(GetWorld(), TEXT("sg.ViewDistanceQuality 4"));
	GEngine->Exec(GetWorld(), TEXT("sg.AntiAliasingQuality 4"));
	GEngine->Exec(GetWorld(), TEXT("sg.ShadowQuality 4"));
	GEngine->Exec(GetWorld(), TEXT("sg.PostProcessQuality 4"));
	GEngine->Exec(GetWorld(), TEXT("sg.TextureQuality 4"));
	GEngine->Exec(GetWorld(), TEXT("sg.FoliageQuality 4"));
	GEngine->Exec(GetWorld(), TEXT("sg.ShadingQuality 4"));
	//极端参数
	GEngine->Exec(GetWorld(), TEXT("r.Streaming.LimitPoolSizeToVRAM 0"));
	GEngine->Exec(GetWorld(), TEXT("r.Streaming.PoolSize 3000"));
	GEngine->Exec(GetWorld(), TEXT("r.streaming.hlodstrategy 2"));
	GEngine->Exec(GetWorld(), TEXT("r.viewdistancescale 10"));
	//部分参数
	GEngine->Exec(GetWorld(), TEXT("r.disablelodfade 1"));
	GEngine->Exec(GetWorld(), TEXT("r.streaming.framesforfullupdate 1"));
	GEngine->Exec(GetWorld(), TEXT("r.Streaming.MaxNumTexturesToStreamPerFrame 0"));
	GEngine->Exec(GetWorld(), TEXT("r.streaming.numstaticcomponentsprocessedperframe 0"));
	GEngine->Exec(GetWorld(), TEXT("r.ScreenPercentage 100"));
	GEngine->Exec(GetWorld(), TEXT("r.ScreenPercentage 100"));
	GEngine->Exec(GetWorld(), TEXT("r.ScreenPercentage 100"));
}

// Called when the game starts or when spawned
void AAnselCapture::BeginPlay()
{
	Super::BeginPlay();

}

// Called every frame
void AAnselCapture::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

}
