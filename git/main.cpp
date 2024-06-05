#include "ltr/include/ltrapi.h"
#include "ltr/include/ltr114api.h"
#include "hiredis/hiredis.h"
#include <windows.h>
#include <stdio.h>
#include <iostream>

#define SIZE 100000
#define INTERVAL 1100

PTLTR114 start_calibration();
float ReadPipe(HANDLE* pipe);
float get_ltr_data(TLTR114* ltr);
void SendPipe(HANDLE* pipe, float number);

PTLTR114 start_calibration() {

	TLTR ltr;
	int error = LTR_Init(&ltr);
	if (error) {
		printf("Init Error with code: %d", error);
		return NULL;
	}
	error = LTR_OpenSvcControl(&ltr, LTRD_ADDR_DEFAULT, LTRD_PORT_DEFAULT);
	if (error) {
		printf("OpenSvcControl Error with code: %d", error);
		return NULL;
	}

	BYTE crates[LTR_CRATES_MAX][LTR_CRATE_SERIAL_SIZE];

	error = LTR_GetCrates(&ltr, *crates);
	if (error) {
		printf("LTR_GetCrates Error with code: %d", error);
		LTR_Close(&ltr);
		return NULL;
	}

	TLTR crate;

	error = LTR_Init(&crate);
	if (error) {
		printf("Init Error with code: %d", error);
		LTR_Close(&ltr);
		return NULL;
	}

	error = LTR_OpenCrate(&crate, LTRD_ADDR_DEFAULT, LTRD_PORT_DEFAULT, LTR_CRATE_IFACE_USB, (const char*)crates[0]);
	if (error) {
		printf("LTR_OpenCrate Error with code: %d", error);
		LTR_Close(&ltr);
		return NULL;
	}
	WORD mid[LTR_MODULES_PER_CRATE_MAX];
	error = LTR_GetCrateModules(&crate, mid);
	if (error) {
		printf("GetCrateModules Error with code: %d", error);
		LTR_Close(&ltr);
		return NULL;
	}
	LTR_Close(&ltr);
	TLTR114* LTR114_array[LTR_MODULES_PER_CRATE_MAX];
	for (int i = 0; i < LTR_MODULES_PER_CRATE_MAX; i++) {
		if (mid[i] == LTR_MID_EMPTY) {
			continue;
		}
		TLTR114 ltr1;
		error = LTR114_Init(&ltr1);
		if (error) {
			printf("LTR114_Init Error with code: %d", error);
			return NULL;
		}
		error = LTR114_Open(&ltr1, SADDR_DEFAULT, SPORT_DEFAULT, (const char*)crates[0], CC_MODULE1);
		if (error) {
			printf("LTR114_Open Error with code: %d", error);
			return NULL;
		}
		LTR114_array[i] = &ltr1;
		error = LTR114_GetConfig(&ltr1);
		if (error) {
			printf("LTR114_GetConfig Error with code: %d", error);
			LTR114_Close(&ltr1);
			return NULL;
		}

	}
	error = LTR114_SetADC(LTR114_array[0]);
	if (error) {
		printf("LTR114_SetADC Error with code: %d", error);
		LTR114_Close(LTR114_array[0]);
		return NULL;
	}

	LTR114_array[0]->FreqDivider = 4;

	error = LTR114_Calibrate(LTR114_array[0]);
	if (error) {
		printf("LTR114_Calibrate Error with code: %d", error);
		LTR114_Close(LTR114_array[0]);
		return NULL;
	}

	error = LTR114_Start(LTR114_array[0]);
	if (error) {
		printf("LTR114_Start Error with code: %d", error);
		LTR114_Close(LTR114_array[0]);
		return NULL;
	}
	return LTR114_array[0];
}

int main() {
	HANDLE MotorPipe = CreateNamedPipe(TEXT("\\\\.\\pipe\\DataPipe"),
		PIPE_ACCESS_OUTBOUND,
		PIPE_TYPE_BYTE | PIPE_READMODE_BYTE | PIPE_WAIT,
		1,
		1024,
		1024,
		0,
		NULL);

	if (MotorPipe == INVALID_HANDLE_VALUE) {
		std::cerr << "Failed to create pipe." << '\n';
		return 1;
	}
	HANDLE  DataPipe = INVALID_HANDLE_VALUE;
	while (DataPipe == INVALID_HANDLE_VALUE) {
		DataPipe = CreateFile(TEXT("\\\\.\\pipe\\MotorPipe"),
			GENERIC_WRITE,
			0,
			NULL,
			OPEN_EXISTING,
			0,
			NULL);
	}
	PTLTR114 ltr = start_calibration();

	float data = 0;
	//X
	while (std::fabs(data + 3) < 1e-6)
	{
		data = ReadPipe(&MotorPipe);
	}
	if (std::fabs(data + 1) < 1e-6)
	{
		float max_intensity = 0;
		float max_intensity_pos = 0;
		while (std::fabs(data + 2) > 1e-6){
			{
				SendPipe(&MotorPipe, -1);
				float photo_data = get_ltr_data(ltr);
				data = ReadPipe(&MotorPipe);
				if(photo_data > max_intensity)
				{
					max_intensity = photo_data;
					max_intensity_pos = data;
				}
			}
		}
		SendPipe(&MotorPipe, max_intensity_pos);

	}
	//Y
	while (std::fabs(data + 3) < 1e-6)
	{
		data = ReadPipe(&MotorPipe);
	}
	if (std::fabs(data + 1) < 1e-6)
	{
		float max_intensity = 0;
		float max_intensity_pos = 0;
		while (std::fabs(data + 2) > 1e-6) {
			{
				SendPipe(&MotorPipe, -1);
				float photo_data = get_ltr_data(ltr);
				data = ReadPipe(&MotorPipe);
				if (photo_data > max_intensity)
				{
					max_intensity = photo_data;
					max_intensity_pos = data;
				}
			}
		}
		SendPipe(&MotorPipe, max_intensity_pos);

	}
}

void SendPipe(HANDLE* pipe, float number)
{
	DWORD bytesWritten;
	bool result = WriteFile(pipe, &number, sizeof(number), &bytesWritten, NULL);

	if (result) {
		std::cout << "Sent number: " << number << std::endl;
	}
	else {
		std::cerr << "Failed to write to pipe." << std::endl;
	}
}

float ReadPipe(HANDLE* pipe){
	float data;
	DWORD bytesRead;
	bool result = true;
	while (result) {
		result = ReadFile(*pipe, &data, sizeof(data), &bytesRead, nullptr);
		if(std::fabs(data + 1) < 1e-6)
		{
			std::cout << "Received: " << data << '\n';
		}
		return data;
		
	}
	return result;
}

float get_ltr_data(TLTR114* ltr)
{
	int array_size = 160;

	DWORD data[SIZE];
	double dest[80];

	printf("1\n");
	int error = LTR114_Recv(ltr, data, NULL, 80 * ltr->FrameLength, 100000);
	printf(" \n%d \n", error);
	if (error < 0) {
		printf("LTR114_Recv Error with code: %d", error);
		LTR114_Close(ltr);
		LTR114_Stop(ltr);

		return 1;
	}
	if (error > 0) {
		error = LTR114_ProcessData(ltr, data, dest, &array_size, LTR114_CORRECTION_MODE_INIT, LTR114_PROCF_VALUE);
		if (error < 0) {
			printf("LTR114_ProcessData Error with code: %d", error);
			LTR114_Close(ltr);
			LTR114_Stop(ltr);

			return 1;
		}
	}
	else if (error < 0) {
		printf("LTR114_ProcessData Error with code: %d", error);
		LTR114_Close(ltr);
		LTR114_Stop(ltr);

		return 1;
	}
	double mean_data;
	double sum = 0;
	for (int j = 0; j < 200; j++)
	{
		sum += dest[j];
	}
	mean_data = sum / 200;

	for (int i = 0; i < 250; i++)
	{
		printf("%d : %lf\n", i, mean_data);
	}
	return mean_data;
	//printf("Work");
}
