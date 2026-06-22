#include "WindSystem.h"
#include "DrawDebugHelpers.h"
#include "Engine/Engine.h"

AWindSystem::AWindSystem()
{
	PrimaryActorTick.bCanEverTick = true;
}

void AWindSystem::BeginPlay()
{
	Super::BeginPlay();
	GustTimer = 0.f;
	TurbulenceTimer = 0.f;
}

void AWindSystem::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	UpdateBaseWind();
	UpdateGusts(DeltaTime);
	UpdateTurbulence(DeltaTime);

	FVector BaseForce = BaseWindDirection.GetSafeNormal() * BaseWindSpeed;
	CurrentWindForce = BaseForce + GustForce + TurbulenceForce;
	CurrentWindSpeed = CurrentWindForce.Size();

	if (CurrentWindSpeed < 100.f)       WindCondition = TEXT("CALM");
	else if (CurrentWindSpeed < 300.f)  WindCondition = TEXT("BREEZY");
	else if (CurrentWindSpeed < 500.f)  WindCondition = TEXT("WINDY");
	else                                WindCondition = TEXT("STORM");

	if (bShowDebug && GEngine)
	{
		GEngine->AddOnScreenDebugMessage(50, 0.f, FColor::Cyan,
			FString::Printf(TEXT("WIND: %.0f cm/s | %s"),
				CurrentWindSpeed, *WindCondition));
	}
}

void AWindSystem::UpdateBaseWind()
{
	float Time = GetWorld() ? GetWorld()->GetTimeSeconds() : 0.f;
	float DirShift = FMath::Sin(Time * 0.1f) * 30.f;
	FRotator Rot(0.f, DirShift, 0.f);
	BaseWindDirection = Rot.RotateVector(BaseWindDirection.GetSafeNormal());
}

void AWindSystem::UpdateGusts(float DeltaTime)
{
	GustTimer += DeltaTime;
	float GustCycle = FMath::Sin(GustTimer * GustFrequency * 0.5f);

	if (GustCycle > 0.7f)
	{
		float RandX = FMath::RandRange(-1.f, 1.f);
		float RandY = FMath::RandRange(-1.f, 1.f);
		FVector GustDir = FVector(RandX, RandY, 0.f).GetSafeNormal();
		GustForce = FMath::VInterpTo(GustForce,
			GustDir * GustIntensity * GustCycle, DeltaTime, 3.f);
	}
	else
	{
		GustForce = FMath::VInterpTo(GustForce,
			FVector::ZeroVector, DeltaTime, 2.f);
	}
}

void AWindSystem::UpdateTurbulence(float DeltaTime)
{
	TurbulenceTimer += DeltaTime;
	float Tx = FMath::PerlinNoise1D(TurbulenceTimer * TurbulenceFrequency * 0.1f);
	float Ty = FMath::PerlinNoise1D(TurbulenceTimer * TurbulenceFrequency * 0.1f + 100.f);
	float Tz = FMath::PerlinNoise1D(TurbulenceTimer * TurbulenceFrequency * 0.1f + 200.f);
	TurbulenceForce = FVector(Tx, Ty, Tz) * TurbulenceIntensity;
}

FVector AWindSystem::GetUpdraftForce(FVector DronePosition)
{
	FVector ToUpdraft = DronePosition - UpdraftCenter;
	ToUpdraft.Z = 0.f;
	float Dist = ToUpdraft.Size();
	if (Dist < UpdraftRadius)
	{
		float Strength = FMath::Clamp(1.f - (Dist / UpdraftRadius), 0.f, 1.f);
		return FVector(0.f, 0.f, UpdraftStrength * Strength);
	}
	return FVector::ZeroVector;
}
