#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "AnselCapture.generated.h"

UCLASS()
class AAnselCapture : public AActor
{
	GENERATED_BODY()

public:
	// Sets default values for this actor's properties
	AAnselCapture();
	UFUNCTION(BlueprintNativeEvent, Category = "Photography")
	void CallSequncePlayer();
	void CallSequncePlayer_Implementation();

	UFUNCTION(BlueprintCallable, Category = "Photography")
	void TestCall();

protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

public:
	// Called every frame
	virtual void Tick(float DeltaTime) override;

};
