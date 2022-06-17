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

void AAnselCapture::TestCall()
{
	CallSequncePlayer();
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
