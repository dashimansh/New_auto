#include "MAVLinkBridge.h"
#include "Sockets.h"
#include "SocketSubsystem.h"
#include "IPAddress.h"
#include "Engine/Engine.h"

AMAVLinkBridge::AMAVLinkBridge()
{
	PrimaryActorTick.bCanEverTick = true;
}

void AMAVLinkBridge::BeginPlay()
{
	Super::BeginPlay();
	if (InitSocket())
	{
		bConnected = true;
		ConnectionStatus = TEXT("CONNECTED");
		if (GEngine)
			GEngine->AddOnScreenDebugMessage(70, 5.f, FColor::Green,
				FString::Printf(TEXT("MAVLink: Connected to %s:%d"),
					*TargetIP, TargetPort));
	}
	else
	{
		ConnectionStatus = TEXT("FAILED");
		if (GEngine)
			GEngine->AddOnScreenDebugMessage(70, 5.f, FColor::Red,
				TEXT("MAVLink: Socket init failed"));
	}
}

void AMAVLinkBridge::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	if (Socket)
	{
		Socket->Close();
		ISocketSubsystem::Get(PLATFORM_SOCKETSUBSYSTEM)->DestroySocket(Socket);
		Socket = nullptr;
	}
	Super::EndPlay(EndPlayReason);
}

void AMAVLinkBridge::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
	HeartbeatTimer += DeltaTime;
	if (HeartbeatTimer >= HeartbeatInterval)
	{
		HeartbeatTimer = 0.f;
		SendHeartbeat();
	}
}

bool AMAVLinkBridge::InitSocket()
{
	ISocketSubsystem* SS = ISocketSubsystem::Get(PLATFORM_SOCKETSUBSYSTEM);
	if (!SS) return false;

	Socket = SS->CreateSocket(NAME_DGram, TEXT("MAVLink"), false);
	if (!Socket) return false;

	TargetAddr = SS->CreateInternetAddr();
	bool bValid = false;
	TargetAddr->SetIp(*TargetIP, bValid);
	TargetAddr->SetPort(TargetPort);
	return bValid;
}

void AMAVLinkBridge::SendPacket(const TArray<uint8>& Data)
{
	if (!Socket || !TargetAddr) return;
	int32 Sent = 0;
	Socket->SendTo(Data.GetData(), Data.Num(), Sent, *TargetAddr);
}

void AMAVLinkBridge::SendPIDGains(
	float RollKp,  float RollKi,  float RollKd,
	float PitchKp, float PitchKi, float PitchKd,
	float YawKp,   float YawKi,   float YawKd,
	float AltKp,   float AltKi,   float AltKd)
{
	if (!bConnected) return;

	TArray<TPair<FString, float>> Params = {
		{TEXT("MC_ROLL_P"),       RollKp},
		{TEXT("MC_ROLLRATE_I"),   RollKi},
		{TEXT("MC_ROLLRATE_D"),   RollKd},
		{TEXT("MC_PITCH_P"),      PitchKp},
		{TEXT("MC_PITCHRATE_I"),  PitchKi},
		{TEXT("MC_PITCHRATE_D"),  PitchKd},
		{TEXT("MC_YAW_P"),        YawKp},
		{TEXT("MC_YAWRATE_I"),    YawKi},
		{TEXT("MC_YAWRATE_D"),    YawKd},
		{TEXT("MPC_Z_P"),         AltKp},
		{TEXT("MPC_Z_VEL_I_ACC"), AltKi},
		{TEXT("MPC_Z_VEL_D_ACC"), AltKd},
	};

	for (auto& P : Params)
		SendPacket(BuildParamSetPacket(P.Key, P.Value));

	if (GEngine)
		GEngine->AddOnScreenDebugMessage(71, 3.f, FColor::Green,
			TEXT("MAVLink: PID Gains Sent"));
}

void AMAVLinkBridge::SendHeartbeat()
{
	if (!bConnected) return;
	SendPacket(BuildHeartbeatPacket());
}

TArray<uint8> AMAVLinkBridge::BuildHeartbeatPacket()
{
	TArray<uint8> P;
	P.SetNum(17);
	P[0]=0xFD; P[1]=9; P[2]=0; P[3]=0; P[4]=0;
	P[5]=1;    P[6]=1; P[7]=0; P[8]=0; P[9]=0;
	P[10]=0; P[11]=0; P[12]=0; P[13]=0;
	P[14]=6; P[15]=8; P[16]=4;
	return P;
}

TArray<uint8> AMAVLinkBridge::BuildParamSetPacket(
	const FString& ParamName, float Value)
{
	TArray<uint8> P;
	P.SetNum(37);
	P[0]=0xFD; P[1]=23; P[2]=0; P[3]=0; P[4]=0;
	P[5]=255;  P[6]=190; P[7]=23; P[8]=0; P[9]=0;

	uint8* FB = reinterpret_cast<uint8*>(&Value);
	P[10]=FB[0]; P[11]=FB[1]; P[12]=FB[2]; P[13]=FB[3];
	P[14]=1; P[15]=1;

	for (int32 i = 0; i < 16; i++)
		P[16+i] = (uint8)(i < ParamName.Len() ? ParamName[i] : 0);

	P[32]=9;
	return P;
}
