#pragma once
#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "WindSystem.generated.h"

UCLASS()
class DOMINOS_API AWindSystem : public AActor
{
	GENERATED_BODY()

public:
	AWindSystem();
	virtual void Tick(float DeltaTime) override;

	FVector GetWindForce() const { return CurrentWindForce; }

	UPROPERTY(EditAnywhere, Category = "Wind|Base")
	FVector BaseWindDirection = FVector(1.f, 0.f, 0.f);

	UPROPERTY(EditAnywhere, Category = "Wind|Base")
	float BaseWindSpeed = 200.f;

	UPROPERTY(EditAnywhere, Category = "Wind|Gust")
	float GustIntensity = 400.f;

	UPROPERTY(EditAnywhere, Category = "Wind|Gust")
	float GustFrequency = 3.f;

	UPROPERTY(EditAnywhere, Category = "Wind|Turbulence")
	float TurbulenceIntensity = 150.f;

	UPROPERTY(EditAnywhere, Category = "Wind|Turbulence")
	float TurbulenceFrequency = 8.f;

	UPROPERTY(EditAnywhere, Category = "Wind|Terrain")
	float UpdraftStrength = 300.f;

	UPROPERTY(EditAnywhere, Category = "Wind|Terrain")
	float UpdraftRadius = 2000.f;

	UPROPERTY(EditAnywhere, Category = "Wind|Terrain")
	FVector UpdraftCenter = FVector(0.f, 0.f, 0.f);

	UPROPERTY(EditAnywhere, Category = "Wind|Debug")
	bool bShowDebug = true;

	UPROPERTY(VisibleAnywhere, Category = "Wind|Status")
	FVector CurrentWindForce = FVector::ZeroVector;

	UPROPERTY(VisibleAnywhere, Category = "Wind|Status")
	float CurrentWindSpeed = 0.f;

	UPROPERTY(VisibleAnywhere, Category = "Wind|Status")
	FString WindCondition = TEXT("CALM");

protected:
	virtual void BeginPlay() override;

private:
	float GustTimer = 0.f;
	float TurbulenceTimer = 0.f;
	FVector GustForce = FVector::ZeroVector;
	FVector TurbulenceForce = FVector::ZeroVector;

	void UpdateBaseWind();
	void UpdateGusts(float DeltaTime);
	void UpdateTurbulence(float DeltaTime);
	FVector GetUpdraftForce(FVector DronePosition);
};
