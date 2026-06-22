#pragma once
#include "CoreMinimal.h"

// Plain C++ struct — do NOT create this as UE class
// Just add AutoTuner.h and AutoTuner.cpp manually in VS2022

struct FAutoTuner
{
	// Current PID gains
	float Kp = 1.f;
	float Ki = 0.f;
	float Kd = 0.1f;

	// State
	bool  bTuning = false;
	bool  bTuned  = false;

	// Z-N internals
	float Ku = 0.f;
	float Tu = 0.f;
	float OscTimer = 0.f;
	float LastPeakTime = 0.f;
	float OscAmplitude = 0.f;
	int32 PeakCount = 0;
	bool  bRising = true;

	// Limits
	float MaxKp = 50.f;
	float MaxKi = 5.f;
	float MaxKd = 20.f;
	float MinKp = 0.1f;

	FString AxisName = TEXT("AXIS");

	void Init(FString Name, float InitKp, float InitKi, float InitKd)
	{
		AxisName = Name;
		Kp = InitKp;
		Ki = InitKi;
		Kd = InitKd;
	}

	float Update(float Error, float DeltaTime);
	void  StartTuning();
	void  StopTuning();
	void  Reset() { Integral = 0.f; PrevError = 0.f; }

private:
	float Integral  = 0.f;
	float PrevError = 0.f;

	void DetectOscillation(float Error, float DeltaTime);
	void ApplyZieglerNichols();
	void AdaptiveAdjust(float Error, float DeltaTime);
};
