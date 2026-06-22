#pragma once
#include "CoreMinimal.h"
#include "AutoTuner.h"

// ─────────────────────────────────────────────────────────────
// FlightController
// Brain of the drone.
// Reads sensor data → runs PID for each axis →
// outputs Roll/Pitch/Yaw/Throttle commands to MotorMixer
// ─────────────────────────────────────────────────────────────

struct FSensorData
{
	// Current drone state (read every tick)
	float ActualRoll     = 0.f;  // degrees
	float ActualPitch    = 0.f;  // degrees
	float ActualYaw      = 0.f;  // degrees
	float ActualAltitude = 0.f;  // cm
	float ActualVelZ     = 0.f;  // cm/s vertical

	FVector ActualPosition   = FVector::ZeroVector;
	FVector ActualVelocity   = FVector::ZeroVector;
	FVector WindDisturbance  = FVector::ZeroVector;
};

struct FMotorOutput
{
	// Normalized 0.0 - 1.0 for each motor
	float FL = 0.f;
	float FR = 0.f;
	float BL = 0.f;
	float BR = 0.f;
};

struct FFlightController
{
	// ── Target setpoints ──
	float TargetRoll     = 0.f;
	float TargetPitch    = 0.f;
	float TargetYaw      = 0.f;
	float TargetAltitude = 0.f;
	float TargetThrottle = 0.5f;

	// ── PID Gains (auto-tuner updates these) ──
	// Roll
	float Roll_Kp  = 8.f;
	float Roll_Ki  = 0.05f;
	float Roll_Kd  = 3.f;

	// Pitch
	float Pitch_Kp = 8.f;
	float Pitch_Ki = 0.05f;
	float Pitch_Kd = 3.f;

	// Yaw
	float Yaw_Kp   = 5.f;
	float Yaw_Ki   = 0.02f;
	float Yaw_Kd   = 2.f;

	// Altitude
	float Alt_Kp   = 15.f;
	float Alt_Ki   = 0.05f;
	float Alt_Kd   = 5.f;

	// ── Auto Tuners ──
	FAutoTuner Tuner_Roll;
	FAutoTuner Tuner_Pitch;
	FAutoTuner Tuner_Yaw;
	FAutoTuner Tuner_Alt;

	// ── State ──
	bool bAutoTuning    = false;
	bool bTuneComplete  = false;
	FString TuneStatus  = TEXT("IDLE");

	// ── PID outputs (for HUD display) ──
	float Out_Roll  = 0.f;
	float Out_Pitch = 0.f;
	float Out_Yaw   = 0.f;
	float Out_Alt   = 0.f;

	// ── Initialize ──
	void Init()
	{
		Tuner_Roll.Init (TEXT("ROLL"),  Roll_Kp,  Roll_Ki,  Roll_Kd);
		Tuner_Pitch.Init(TEXT("PITCH"), Pitch_Kp, Pitch_Ki, Pitch_Kd);
		Tuner_Yaw.Init  (TEXT("YAW"),   Yaw_Kp,   Yaw_Ki,   Yaw_Kd);
		Tuner_Alt.Init  (TEXT("ALT"),   Alt_Kp,   Alt_Ki,   Alt_Kd);
	}

	// ── Main update — call every Tick ──
	// Returns motor commands via FMotorOutput
	FMotorOutput Update(const FSensorData& Sensors, float DeltaTime);

	// ── Auto tune control ──
	void StartAutoTune();
	void StopAutoTune();

	// ── Reset all integrators ──
	void Reset()
	{
		Tuner_Roll.Reset();
		Tuner_Pitch.Reset();
		Tuner_Yaw.Reset();
		Tuner_Alt.Reset();
	}

private:
	// Internal PID states (used when NOT auto-tuning)
	struct FPIDState
	{
		float Integral  = 0.f;
		float PrevError = 0.f;
		void Reset() { Integral = 0.f; PrevError = 0.f; }
	};

	FPIDState PID_Roll;
	FPIDState PID_Pitch;
	FPIDState PID_Yaw;
	FPIDState PID_Alt;

	float ComputePID(FPIDState& State,
		float Kp, float Ki, float Kd,
		float Error, float DeltaTime);

	void SyncGainsFromTuners();
	void UpdateAutoTuneStatus();

	// Test input for auto-tune axis testing
	float TuneTestTimer    = 0.f;
	float TuneTestDuration = 3.f;  // seconds per axis test
	int32 TuneAxisIndex    = 0;    // 0=Roll 1=Pitch 2=Yaw 3=Alt
};
