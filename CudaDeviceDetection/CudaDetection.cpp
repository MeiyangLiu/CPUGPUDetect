#include "CudaDetection.h"

#include <iostream>
#include <stdexcept>

#include "atlstr.h"  //for 	 BOOL WINAPI DllMain

using namespace std;

CudaDetection::CudaDetection() { }
CudaDetection::~CudaDetection() { }

#define PCI_BUS_LEN 64

bool CudaDetection::QueryDevices() {
	const unsigned int UUID_SIZE = 128;
	try {
		int device_count;
		CUDA_SAFE_CALL(cudaGetDeviceCount(&device_count));
		NVML_SAFE_CALL(nvmlInit());
		for (unsigned int i = 0; i < device_count; ++i) {
			CudaDevice cudaDevice;

			cudaDeviceProp props;
			CUDA_SAFE_CALL(cudaGetDeviceProperties(&props, i));
			char pciBusID[PCI_BUS_LEN];
			CUDA_SAFE_CALL(cudaDeviceGetPCIBusId(pciBusID, PCI_BUS_LEN, i));

			// serial stuff
			nvmlPciInfo_t pciInfo;
			nvmlDevice_t device_t;
			char uuid[UUID_SIZE];
			//NVML_SAFE_CALL(nvmlDeviceGetHandleByIndex(i, &device_t)); // this is wrong
			NVML_SAFE_CALL(nvmlDeviceGetHandleByPciBusId(pciBusID, &device_t)); // use pciBusID to get the right version
			NVML_SAFE_CALL(nvmlDeviceGetPciInfo(device_t, &pciInfo));
			NVML_SAFE_CALL(nvmlDeviceGetUUID(device_t, uuid, UUID_SIZE));

			// init device info
			cudaDevice.DeviceID = i;
			cudaDevice.pciBusID = props.pciBusID;
			cudaDevice.VendorName = getVendorString(pciInfo);
			cudaDevice.DeviceName = props.name;
			cudaDevice.SMVersionString = to_string(props.major) + "." + to_string(props.minor);
			cudaDevice.SM_major = props.major;
			cudaDevice.SM_minor = props.minor;
			cudaDevice.UUID = uuid;
			cudaDevice.DeviceGlobalMemory = props.totalGlobalMem;
			cudaDevice.pciDeviceId = pciInfo.pciDeviceId;
			cudaDevice.pciSubSystemId = pciInfo.pciSubSystemId;
			cudaDevice.SMX = props.multiProcessorCount;
			cudaDevice.VendorID = getVendorId(pciInfo);

			_cudaDevices.push_back(cudaDevice);
		}
		NVML_SAFE_CALL(nvmlShutdown());
	}
	catch (runtime_error &err) {
		_errorMsgs.push_back(err.what());
		return false;
	}
	return true;
}

std::map<uint16_t, std::string> CudaDetection::_VENDOR_NAMES = {
	{ 0x1043, "ASUS" },
	{ 0x107D, "Leadtek" },
	{ 0x10B0, "Gainward" },
	{ 0x10DE, "NVIDIA" },
	{ 0x1458, "Gigabyte" },
	{ 0x1462, "MSI" },
	{ 0x154B, "PNY" },
	{ 0x1682, "XFX" },
	{ 0x196D, "Club3D" },
	{ 0x19DA, "Zotac" },
	{ 0x19F1, "BFG" },
	{ 0x1ACC, "PoV" },
	{ 0x1B4C, "KFA2" },
	{ 0x3842, "EVGA" },
	{ 0x7377, "Colorful" },
	{ 0, "" }
};

uint16_t CudaDetection::getVendorId(nvmlPciInfo_t &nvmlPciInfo) {
	uint16_t vendorId = 0;
	vendorId = nvmlPciInfo.pciDeviceId & 0xFFFF;
	if (vendorId == 0x10DE && nvmlPciInfo.pciSubSystemId) {
		vendorId = nvmlPciInfo.pciSubSystemId & 0xFFFF;
	}
	return vendorId;
}

std::string CudaDetection::getVendorString(nvmlPciInfo_t &nvmlPciInfo) {
	auto venId = getVendorId(nvmlPciInfo);
	for (const auto &vpair : _VENDOR_NAMES) {
		if (vpair.first == venId) return vpair.second;
	}
	return "UNKNOWN";
}

void CudaDetection::PrintDevicesJson() {
	cout << "[" << endl;
	for (int i = 0; i < _cudaDevices.size() - 1; ++i) {
		json_print(_cudaDevices[i]);
		cout << "," << endl;
	}
	json_print(_cudaDevices[_cudaDevices.size() - 1]);
	cout << endl << "]" << endl;
}


void CudaDetection::json_print(CudaDevice &dev) {
	cout << "\t{" << endl;
	cout << "\t\t\"DeviceID\" : " << dev.DeviceID << "," << endl; // num
	cout << "\t\t\"pciBusID\" : " << dev.pciBusID << "," << endl; // num
	cout << "\t\t\"VendorID\" : " << dev.VendorID << "," << endl; // num
	cout << "\t\t\"VendorName\" : \"" << dev.VendorName << "\"," << endl; // string
	cout << "\t\t\"DeviceName\" : \"" << dev.DeviceName << "\"," << endl; // string
	cout << "\t\t\"SMVersionString\" : \"" << dev.SMVersionString << "\"," << endl;  // string
	cout << "\t\t\"SM_major\" : " << dev.SM_major << "," << endl; // num
	cout << "\t\t\"SM_minor\" : " << dev.SM_minor << "," << endl; // num
	cout << "\t\t\"UUID\" : \"" << dev.UUID << "\"," << endl;  // string
	cout << "\t\t\"DeviceGlobalMemory\" : " << dev.DeviceGlobalMemory << "," << endl; // num
	cout << "\t\t\"pciDeviceId\" : " << dev.pciDeviceId << "," << endl; // num
	cout << "\t\t\"pciSubSystemId\" : " << dev.pciSubSystemId << "," << endl; // num
	cout << "\t\t\"SMX\" : " << dev.SMX << endl; // num
	cout << "\t}";
}

// non human readable print
void CudaDetection::PrintDevicesJson_d() {
	cout << "[";
	for (int i = 0; i < _cudaDevices.size() - 1; ++i) {
		json_print_d(_cudaDevices[i]);
		cout << ",";
	}
	json_print_d(_cudaDevices[_cudaDevices.size() - 1]);
	cout << "]" << endl;
}


void CudaDetection::json_print_d(CudaDevice &dev) {
	cout << "{";
	cout << "\"DeviceID\" : " << dev.DeviceID << ","; // num
	cout << "\"pciBusID\" : " << dev.pciBusID << ","; // num
	cout << "\"VendorID\" : " << dev.VendorID << ","; // num
	cout << "\"VendorName\" : \"" << dev.VendorName << "\","; // string
	cout << "\"DeviceName\" : \"" << dev.DeviceName << "\","; // string
	cout << "\"SMVersionString\" : \"" << dev.SMVersionString << "\",";  // string
	cout << "\"SM_major\" : " << dev.SM_major << ","; // num
	cout << "\"SM_minor\" : " << dev.SM_minor << ","; // num
	cout << "\"UUID\" : \"" << dev.UUID << "\",";  // string
	cout << "\"DeviceGlobalMemory\" : " << dev.DeviceGlobalMemory << ","; // num
	cout << "\"pciDeviceId\" : " << dev.pciDeviceId << ","; // num
	cout << "\"pciSubSystemId\" : " << dev.pciSubSystemId << ","; // num
	cout << "\"SMX\" : " << dev.SMX; // num
	cout << "}";
}



//modify the return value
std::string CudaDetection::GetDevicesJson() {
	string deviceJson;
	/*cout << "[" << endl;*/
	deviceJson = "[\n";

	for (int i = 0; i < _cudaDevices.size() - 1; ++i) {
		deviceJson += json_makeup(_cudaDevices[i]);
		//cout << "," << endl;
		deviceJson += ",\n";
	}
	deviceJson += json_makeup(_cudaDevices[_cudaDevices.size() - 1]);
	//cout << endl << "]" << endl;
	deviceJson += "\n]\n";
	return deviceJson;
}

std::string  CudaDetection::json_makeup(CudaDevice &dev) {
	string cudaJson;

	cudaJson += "\t{\n";
	cudaJson += "\t\t\"DeviceID\" : " + to_string(dev.DeviceID) + ",\n"; // num
	cudaJson += "\t\t\"pciBusID\" : " + to_string(dev.pciBusID) + "," + "\n"; // num
	cudaJson += "\t\t\"VendorID\" : " + to_string(dev.VendorID) + "," + "\n"; // num
	cudaJson += "\t\t\"VendorName\" : \"" + dev.VendorName + "\"," + "\n"; // string
	cudaJson += "\t\t\"DeviceName\" : \"" + dev.DeviceName + "\"," + "\n"; // string
	cudaJson += "\t\t\"SMVersionString\" : \"" + dev.SMVersionString + "\"," + "\n";  // string
	cudaJson += "\t\t\"SM_major\" : " + to_string(dev.SM_major) + "," + "\n"; // num
	cudaJson += "\t\t\"SM_minor\" : " + to_string(dev.SM_minor) + "," + "\n"; // num
	cudaJson += "\t\t\"UUID\" : \"" + dev.UUID + "\"," + "\n";  // string
	cudaJson += "\t\t\"DeviceGlobalMemory\" : " + to_string(dev.DeviceGlobalMemory) + "," + "\n"; // num
	cudaJson += "\t\t\"pciDeviceId\" : " + to_string(dev.pciDeviceId) + "," + "\n"; // num
	cudaJson += "\t\t\"pciSubSystemId\" : " + to_string(dev.pciSubSystemId) + "," + "\n"; // num
	cudaJson += "\t\t\"SMX\" : " + to_string(dev.SMX) + "\n"; // num
	cudaJson += "\t}";
	return cudaJson;
}

// non human readable print
string CudaDetection::GetDevicesJson_d() {
	string deviceJson;
	//cout << "[";
	deviceJson = "[";
	for (int i = 0; i < _cudaDevices.size() - 1; ++i) {
		deviceJson += json_makeup_d(_cudaDevices[i]);
		//cout << ",";
		deviceJson += ",";
	}
	deviceJson += json_makeup_d(_cudaDevices[_cudaDevices.size() - 1]);
	//cout << "]" << endl;
	deviceJson += "]\n";
	return deviceJson;
}

string CudaDetection::json_makeup_d(CudaDevice &dev) {
	
	string cudaJson;
	cudaJson += "{";
	cudaJson += "\"DeviceID\" : " + to_string(dev.DeviceID) + ","; // num
	cudaJson += "\"pciBusID\" : " + to_string(dev.pciBusID) + ","; // num
	cudaJson += "\"VendorID\" : " + to_string(dev.VendorID) + ","; // num
	cudaJson += "\"VendorName\" : \"" + dev.VendorName + "\","; // string
	cudaJson += "\"DeviceName\" : \"" + dev.DeviceName + "\","; // string
	cudaJson += "\"SMVersionString\" : \"" + dev.SMVersionString + "\",";  // string
	cudaJson += "\"SM_major\" : " + to_string(dev.SM_major) + ","; // num
	cudaJson += "\"SM_minor\" : " + to_string(dev.SM_minor) + ","; // num
	cudaJson += "\"UUID\" : \"" + dev.UUID + "\",";  // string
	cudaJson += "\"DeviceGlobalMemory\" : " + to_string(dev.DeviceGlobalMemory) + ","; // num
	cudaJson += "\"pciDeviceId\" : " + to_string(dev.pciDeviceId) + ","; // num
	cudaJson += "\"pciSubSystemId\" : " + to_string(dev.pciSubSystemId) + ","; // num
	cudaJson += "\"SMX\" : " + to_string(dev.SMX); // num
	cudaJson += "}";
	return cudaJson;
}


extern "C"
{
	__declspec(dllexport) bool  _QueryDevices()
	{
		CudaDetection cuda;
		return cuda.QueryDevices();
	}
	__declspec(dllexport) char* __cdecl _GetDevicesJson_d()
	{
		CudaDetection cuda;
		static string deviceJson;
		if (cuda.QueryDevices()) {
			//const char* p = amd.GetDevicesJsonDirty().data();
			deviceJson = cuda.GetDevicesJson_d();
		}
		char *charJson = (char*)deviceJson.c_str();
		return charJson;
	}
	

}


BOOL WINAPI DllMain(
	HINSTANCE hinstDLL,  // handle to DLL module
	DWORD fdwReason,     // reason for calling function
	LPVOID lpReserved)  // reserved
{
	// Perform actions based on the reason for calling.
	switch (fdwReason)
	{
	case DLL_PROCESS_ATTACH:
		// Initialize once for each new process.
		// Return FALSE to fail DLL load.
		break;

	case DLL_THREAD_ATTACH:
		// Do thread-specific initialization.
		break;

	case DLL_THREAD_DETACH:
		// Do thread-specific cleanup.
		break;

	case DLL_PROCESS_DETACH:
		// Perform any necessary cleanup.
		break;
	}
	return TRUE;  // Successful DLL_PROCESS_ATTACH.
}

//int main(int argc, char* argv[]) {
//
//	CudaDetection amd;
//	string deviceJson;
//	if (amd.QueryDevices()) {
//		//const char* p = amd.GetDevicesJsonDirty().data();
//		deviceJson = amd.GetDevicesJson();
//	}
//	char *charJson = (char*)deviceJson.c_str();
//	cout << charJson;
//	return 0;
//}