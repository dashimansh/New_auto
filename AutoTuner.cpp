#include "AutoTuner.h"
#include "Engine/Engine.h"

float FAutoTuner::Update(float Error, float DeltaTime)
{
	if (bTuning)
	{
		DetectOscillation(Error, DeltaTime);
		AdaptiveAdjust(Error, DeltaTime);
	}

	float P = Kp * Error;
	Integral += Error * DeltaTime;
	Integral = FMath::Clamp(Integral, -50.f, 50.f);
	float Derivative = (DeltaTime > 0.f)
		? (Error - PrevError) / DeltaTime : 0.f;
	PrevError = Error;

	return P + (Ki * Integral) + (Kd * Derivative);
}

void FAutoTuner::StartTuning()
{
	bTuning = true;
	bTuned  = false;
	OscTimer = 0.f;
	PeakCount = 0;
	OscAmplitude = 0.f;
	bRising = true;
	Kp = FMath::Max(Kp * 1.5f, 2.f);
	Ki = 0.f;
	Kd = 0.f;

	if (GEngine)
		GEngine->AddOnScreenDebugMessage(60, 3.f, FColor::Yellow,
			FString::Printf(TEXT("[%s] AUTO-TUNE STARTED"), *AxisName));
}

void FAutoTuner::StopTuning()
{
	bTuning = false;
	bTuned  = true;
	Reset();

	if (GEngine)
		GEngine->AddOnScreenDebugMessage(61, 5.f, FColor::Green,
			FString::Printf(TEXT("[%s] TUNED: Kp=%.3f Ki=%.3f Kd=%.3f"),
				*AxisName, Kp, Ki, Kd));
}

void FAutoTuner::DetectOscillation(float Error, float DeltaTime)
{
	OscTimer += DeltaTime;
	bool bCurrentRising = (Error > PrevError);

	if (bCurrentRising != bRising)
	{
		float PeakValue = FMath::Abs(Error);
		OscAmplitude = FMath::Max(OscAmplitude, PeakValue);

		if (PeakCount > 0)
		{
			float PeriodEstimate = (OscTimer - LastPeakTime) * 2.f;
			if (PeakValue > 5.f && PeriodEstimate > 0.1f)
				Tu = Tu * 0.7f + PeriodEstimate * 0.3f;
		}

		LastPeakTime = OscTimer;
		PeakCount++;
		bRising = bCurrentRising;

		if (PeakCount >= 6 && OscAmplitude > 3.f)
		{
			Ku = Kp;
			ApplyZieglerNichols();
			StopTuning();
		}
	}

	if (OscAmplitude > 500.f)
	{
		Kp *= 0.8f;
		OscAmplitude = 0.f;
	}

	if (OscTimer > 10.f && PeakCount < 3)
	{
		Kp = FMath::Clamp(Kp * 1.3f, MinKp, MaxKp);
		OscTimer = 0.f;
		PeakCount = 0;
	}
}

void FAutoTuner::ApplyZieglerNichols()
{
	if (Tu < 0.01f) Tu = 0.5f;
	Kp = FMath::Clamp(0.6f  * Ku,        MinKp, MaxKp);
	Ki = FMath::Clamp(2.f   * Kp / Tu,   0.f,   MaxKi);
	Kd = FMath::Clamp(Kp    * Tu / 8.f,  0.f,   MaxKd);

	if (GEngine)
		GEngine->AddOnScreenDebugMessage(62, 8.f, FColor::Cyan,
			FString::Printf(
				TEXT("[%s] Z-N: Ku=%.2f Tu=%.2f -> Kp=%.3f Ki=%.3f Kd=%.3f"),
				*AxisName, Ku, Tu, Kp, Ki, Kd));
}

void FAutoTuner::AdaptiveAdjust(float Error, float DeltaTime)
{
	float AbsError = FMath::Abs(Error);
	float AbsPrev  = FMath::Abs(PrevError);

	if (!bTuned)
	{
		if (AbsError > AbsPrev + 10.f)
			Kp = FMath::Clamp(Kp * 1.01f, MinKp, MaxKp);
		else if (AbsError < AbsPrev - 10.f && AbsError < 5.f)
			Kd = FMath::Clamp(Kd * 1.005f, 0.f, MaxKd);
	}
}
