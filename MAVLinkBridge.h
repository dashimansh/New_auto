#pragma once
#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "MAVLinkBridge.generated.h"

UCLASS()
class DOMINOS_API AMAVLinkBridge : public AActor
{
	GENERATED_BODY()

public:
	AMAVLinkBridge();

	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
	virtual void Tick(float DeltaTime) override;

	UPROPERTY(EditAnywhere, Category = "MAVLink")
	FString TargetIP = TEXT("127.0.0.1");

	UPROPERTY(EditAnywhere, Category = "MAVLink")
	int32 TargetPort = 14550;

	UPROPERTY(VisibleAnywhere, Category = "MAVLink")
	bool bConnected = false;

	UPROPERTY(VisibleAnywhere, Category = "MAVLink")
	FString ConnectionStatus = TEXT("DISCONNECTED");

	UPROPERTY(EditAnywhere, Category = "MAVLink")
	float HeartbeatInterval = 1.f;

	void SendPIDGains(
		float RollKp,  float RollKi,  float RollKd,
		float PitchKp, float PitchKi, float PitchKd,
		float YawKp,   float YawKi,   float YawKd,
		float AltKp,   float AltKi,   float AltKd);

	void SendHeartbeat();

private:
	class FSocket* Socket = nullptr;
	TSharedPtr<class FInternetAddr> TargetAddr;
	float HeartbeatTimer = 0.f;

	bool InitSocket();
	void SendPacket(const TArray<uint8>& Data);
	TArray<uint8> BuildParamSetPacket(const FString& ParamName, float Value);
	TArray<uint8> BuildHeartbeatPacket();
};
