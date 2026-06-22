#include "FlightController.h"
#include "Engine/Engine.h"

// ─────────────────────────────────────────────────────────────
// Main Update — runs every Tick
// ─────────────────────────────────────────────────────────────
FMotorOutput FFlightController::Update(
	const FSensorData& Sensors, float DeltaTime)
{
	// ── Step 1: Calculate errors for each axis ──
	float RollError  = TargetRoll     - Sensors.ActualRoll;
	float PitchError = TargetPitch    - Sensors.ActualPitch;
	float YawError   = TargetYaw      - Sensors.ActualYaw;
	float AltError   = TargetAltitude - Sensors.ActualAltitude;

	// Normalize yaw error to -180/+180
	while (YawError >  180.f) YawError -= 360.f;
	while (YawError < -180.f) YawError += 360.f;

	// ── Step 2: Run PID for each axis ──
	if (bAutoTuning)
	{
		// Auto-tuner runs PID AND adjusts gains
		Out_Roll  = Tuner_Roll.Update (RollError,  DeltaTime);
		Out_Pitch = Tuner_Pitch.Update(PitchError, DeltaTime);
		Out_Yaw   = Tuner_Yaw.Update  (YawError,   DeltaTime);
		Out_Alt   = Tuner_Alt.Update  (AltError,   DeltaTime);

		// Sync improved gains back
		SyncGainsFromTuners();
		UpdateAutoTuneStatus();
	}
	else
	{
		// Standard PID with fixed gains
		Out_Roll  = ComputePID(PID_Roll,  Roll_Kp,  Roll_Ki,  Roll_Kd,  RollError,  DeltaTime);
		Out_Pitch = ComputePID(PID_Pitch, Pitch_Kp, Pitch_Ki, Pitch_Kd, PitchError, DeltaTime);
		Out_Yaw   = ComputePID(PID_Yaw,   Yaw_Kp,   Yaw_Ki,   Yaw_Kd,   YawError,   DeltaTime);
		Out_Alt   = ComputePID(PID_Alt,   Alt_Kp,   Alt_Ki,   Alt_Kd,   AltError,   DeltaTime);
	}

	// ── Step 3: Clamp outputs ──
	Out_Roll  = FMath::Clamp(Out_Roll,  -1.f, 1.f);
	Out_Pitch = FMath::Clamp(Out_Pitch, -1.f, 1.f);
	Out_Yaw   = FMath::Clamp(Out_Yaw,  -1.f, 1.f);
	Out_Alt   = FMath::Clamp(Out_Alt,  -1.f, 1.f);

	// ── Step 4: Motor Mixer ──
	// Converts Roll/Pitch/Yaw/Throttle → 4 motor speeds
	//
	//     FL(CW)    FR(CCW)
	//        \      /
	//         ------
	//        /      \
	//     BL(CCW)   BR(CW)
	//
	// Pitch forward  → BL,BR faster  / FL,FR slower
	// Roll right     → FL,BL faster  / FR,BR slower
	// Yaw right      → FL,BR faster  / FR,BL slower (torque)

	FMotorOutput Motors;
	float T = FMath::Clamp(TargetThrottle, 0.f, 1.f);

	Motors.FL = FMath::Clamp(T + Out_Pitch + Out_Roll - Out_Yaw, 0.f, 1.f);
	Motors.FR = FMath::Clamp(T + Out_Pitch - Out_Roll + Out_Yaw, 0.f, 1.f);
	Motors.BL = FMath::Clamp(T - Out_Pitch + Out_Roll + Out_Yaw, 0.f, 1.f);
	Motors.BR = FMath::Clamp(T - Out_Pitch - Out_Roll - Out_Yaw, 0.f, 1.f);

	// ── Step 5: Debug display ──
	if (GEngine)
	{
		GEngine->AddOnScreenDebugMessage(80, 0.f, FColor::Green,
			FString::Printf(TEXT("FC | R:%.2f P:%.2f Y:%.2f A:%.2f"),
				Out_Roll, Out_Pitch, Out_Yaw, Out_Alt));
		GEngine->AddOnScreenDebugMessage(81, 0.f, FColor::Cyan,
			FString::Printf(TEXT("MOTORS | FL:%.2f FR:%.2f BL:%.2f BR:%.2f"),
				Motors.FL, Motors.FR, Motors.BL, Motors.BR));
		GEngine->AddOnScreenDebugMessage(82, 0.f, FColor::Yellow,
			FString::Printf(TEXT("GAINS | Rp:%.2f Pp:%.2f Yp:%.2f Ap:%.2f"),
				Roll_Kp, Pitch_Kp, Yaw_Kp, Alt_Kp));
		if (bAutoTuning)
			GEngine->AddOnScreenDebugMessage(83, 0.f, FColor::Orange,
				FString::Printf(TEXT("AUTO-TUNE: %s"), *TuneStatus));
	}

	return Motors;
}

// ─────────────────────────────────────────────────────────────
// Standard PID compute
// ─────────────────────────────────────────────────────────────
float FFlightController::ComputePID(
	FPIDState& State,
	float Kp, float Ki, float Kd,
	float Error, float DeltaTime)
{
	// Proportional
	float P = Kp * Error;

	// Integral — accumulates over time
	// Eliminates steady-state error
	State.Integral += Error * DeltaTime;
	State.Integral  = FMath::Clamp(State.Integral, -50.f, 50.f);
	float I = Ki * State.Integral;

	// Derivative — rate of change of error
	// Reduces overshoot
	float Derivative = (DeltaTime > 0.f)
		? (Error - State.PrevError) / DeltaTime : 0.f;
	State.PrevError = Error;
	float D = Kd * Derivative;

	return P + I + D;
}

// ─────────────────────────────────────────────────────────────
// Auto Tune — Start
// ─────────────────────────────────────────────────────────────
void FFlightController::StartAutoTune()
{
	bAutoTuning   = true;
	bTuneComplete = false;
	TuneStatus    = TEXT("TUNING ROLL...");
	TuneAxisIndex = 0;
	TuneTestTimer = 0.f;

	// Start all tuners
	Tuner_Roll.StartTuning();
	Tuner_Pitch.StartTuning();
	Tuner_Yaw.StartTuning();
	Tuner_Alt.StartTuning();

	if (GEngine)
		GEngine->AddOnScreenDebugMessage(84, 5.f, FColor::Yellow,
			TEXT(">> AUTO-TUNE STARTED <<"));
}

// ─────────────────────────────────────────────────────────────
// Auto Tune — Stop
// ─────────────────────────────────────────────────────────────
void FFlightController::StopAutoTune()
{
	bAutoTuning = false;
	TuneStatus  = TEXT("STOPPED");

	Tuner_Roll.StopTuning();
	Tuner_Pitch.StopTuning();
	Tuner_Yaw.StopTuning();
	Tuner_Alt.StopTuning();

	SyncGainsFromTuners();

	if (GEngine)
		GEngine->AddOnScreenDebugMessage(84, 5.f, FColor::Green,
			FString::Printf(
				TEXT("TUNED | Roll Kp=%.2f | Pitch Kp=%.2f | Yaw Kp=%.2f | Alt Kp=%.2f"),
				Roll_Kp, Pitch_Kp, Yaw_Kp, Alt_Kp));
}

// ─────────────────────────────────────────────────────────────
// Sync tuned gains back to exposed variables
// ─────────────────────────────────────────────────────────────
void FFlightController::SyncGainsFromTuners()
{
	// Roll
	Roll_Kp  = Tuner_Roll.Kp;
	Roll_Ki  = Tuner_Roll.Ki;
	Roll_Kd  = Tuner_Roll.Kd;

	// Pitch
	Pitch_Kp = Tuner_Pitch.Kp;
	Pitch_Ki = Tuner_Pitch.Ki;
	Pitch_Kd = Tuner_Pitch.Kd;

	// Yaw
	Yaw_Kp   = Tuner_Yaw.Kp;
	Yaw_Ki   = Tuner_Yaw.Ki;
	Yaw_Kd   = Tuner_Yaw.Kd;

	// Altitude
	Alt_Kp   = Tuner_Alt.Kp;
	Alt_Ki   = Tuner_Alt.Ki;
	Alt_Kd   = Tuner_Alt.Kd;
}

// ─────────────────────────────────────────────────────────────
// Update tune status display
// ─────────────────────────────────────────────────────────────
void FFlightController::UpdateAutoTuneStatus()
{
	bool bRollDone  = Tuner_Roll.bTuned;
	bool bPitchDone = Tuner_Pitch.bTuned;
	bool bYawDone   = Tuner_Yaw.bTuned;
	bool bAltDone   = Tuner_Alt.bTuned;

	if (!bRollDone)
		TuneStatus = TEXT("TUNING ROLL...");
	else if (!bPitchDone)
		TuneStatus = TEXT("TUNING PITCH...");
	else if (!bYawDone)
		TuneStatus = TEXT("TUNING YAW...");
	else if (!bAltDone)
		TuneStatus = TEXT("TUNING ALT...");
	else
	{
		TuneStatus    = TEXT("COMPLETE");
		bTuneComplete = true;
		bAutoTuning   = false;

		if (GEngine)
			GEngine->AddOnScreenDebugMessage(85, 8.f, FColor::Green,
				TEXT(">> ALL AXES TUNED — GAINS READY FOR REAL DRONE <<"));
	}
}
