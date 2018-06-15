#include "AMDOpenCLDeviceDetection.h"

#include <iostream>
#include <stdexcept>
#include <utility>

using namespace std;
using namespace cl;

AMDOpenCLDeviceDetection::AMDOpenCLDeviceDetection()
{
}

AMDOpenCLDeviceDetection::~AMDOpenCLDeviceDetection()
{
}

vector<Platform> AMDOpenCLDeviceDetection::getPlatforms() {
	vector<Platform> platforms;
	try {
		Platform::get(&platforms);
	}
	catch (Error const& err) {
#if defined(CL_PLATFORM_NOT_FOUND_KHR)
		if (err.err() == CL_PLATFORM_NOT_FOUND_KHR)
			cout << "No OpenCL platforms found" << endl;
		else
#endif
			throw err;
	}
	return platforms;
}

vector<Device> AMDOpenCLDeviceDetection::getDevices(vector<Platform> const& _platforms, unsigned _platformId) {
	vector<Device> devices;
	try {
		_platforms[_platformId].getDevices(/*CL_DEVICE_TYPE_CPU| */CL_DEVICE_TYPE_GPU | CL_DEVICE_TYPE_ACCELERATOR, &devices);
	}
	catch (Error const& err) {
		// if simply no devices found return empty vector
		if (err.err() != CL_DEVICE_NOT_FOUND)
			throw err;
	}
	return devices;
}

string AMDOpenCLDeviceDetection::StringnNullTerminatorFix(const string& str) {
	return string(str.c_str(), strlen(str.c_str()));
}

bool AMDOpenCLDeviceDetection::QueryDevices() {
	try {
		// get platforms
		auto platforms = getPlatforms();
		if (platforms.empty()) {
			cout << "No OpenCL platforms found" << endl;
			return false;
		}
		else {
			for (auto i_pId = 0u; i_pId < platforms.size(); ++i_pId) {
				string platformName = StringnNullTerminatorFix(platforms[i_pId].getInfo<CL_PLATFORM_NAME>());
				if (std::find(_platformNames.begin(), _platformNames.end(), platformName) == _platformNames.end()) {
					JsonLog current;
					_platformNames.push_back(platformName);
					// new
					current.PlatformName = platformName;
					current.PlatformNum = i_pId;

					// not the best way but it should work
					bool isAMD = platformName.find("AMD", 0) != string::npos;
					auto clDevs = getDevices(platforms, i_pId);
					for (auto i_devId = 0u; i_devId < clDevs.size(); ++i_devId) {
						OpenCLDevice curDevice;
						curDevice.DeviceID = i_devId;
						curDevice._CL_DEVICE_NAME = StringnNullTerminatorFix(clDevs[i_devId].getInfo<CL_DEVICE_NAME>());
						switch (clDevs[i_devId].getInfo<CL_DEVICE_TYPE>()) {
						case CL_DEVICE_TYPE_CPU:
							curDevice._CL_DEVICE_TYPE = "CPU";
							break;
						case CL_DEVICE_TYPE_GPU:
							curDevice._CL_DEVICE_TYPE = "GPU";
							break;
						case CL_DEVICE_TYPE_ACCELERATOR:
							curDevice._CL_DEVICE_TYPE = "ACCELERATOR";
							break;
						default:
							curDevice._CL_DEVICE_TYPE = "DEFAULT";
							break;
						}

						curDevice._CL_DEVICE_GLOBAL_MEM_SIZE = clDevs[i_devId].getInfo<CL_DEVICE_GLOBAL_MEM_SIZE>();
						curDevice._CL_DEVICE_VENDOR = StringnNullTerminatorFix(clDevs[i_devId].getInfo<CL_DEVICE_VENDOR>());
						curDevice._CL_DEVICE_VERSION = StringnNullTerminatorFix(clDevs[i_devId].getInfo<CL_DEVICE_VERSION>());
						curDevice._CL_DRIVER_VERSION = StringnNullTerminatorFix(clDevs[i_devId].getInfo<CL_DRIVER_VERSION>());

						// AMD topology get Bus No
						if (isAMD) {
							cl_device_topology_amd topology = clDevs[i_devId].getInfo<CL_DEVICE_TOPOLOGY_AMD>();
							if (topology.raw.type == CL_DEVICE_TOPOLOGY_TYPE_PCIE_AMD) {
								curDevice.AMD_BUS_ID = (int)topology.pcie.bus;
							}
						}

						current.Devices.push_back(curDevice);
					}
					_devicesPlatformsDevices.push_back(current);
				}
			}
		}
	}
	catch (exception &ex) {
		// TODO
		cout << "AMDOpenCLDeviceDetection::QueryDevices() exception: " << ex.what() << endl;
		return false;
	}

	return true;
}

#define COMMA(num) ((--num > 0 ? "," : ""))

// this function is hardcoded and horrable
void AMDOpenCLDeviceDetection::PrintDevicesJson() {
	cout << "[" << endl;

	{
		int devPlatformsComma = _devicesPlatformsDevices.size();
		for (const auto &jsonLog : _devicesPlatformsDevices) {
			cout << "\t{" << endl;
			cout << "\t\t\"PlatformName\": \"" << jsonLog.PlatformName << "\"" << "," << endl;
			cout << "\t\t\"PlatformNum\": " << jsonLog.PlatformNum << "," << endl;
			cout << "\t\t\"Devices\" : [" << endl;
			// device print
			int devComma = jsonLog.Devices.size();
			for (const auto &dev : jsonLog.Devices) {
				cout << "\t\t\t{" << endl;
				cout << "\t\t\t\t\"" << "DeviceID" << "\" : " << dev.DeviceID << "," << endl; // num
				cout << "\t\t\t\t\"" << "AMD_BUS_ID" << "\" : " << dev.AMD_BUS_ID << "," << endl; // num
				cout << "\t\t\t\t\"" << "_CL_DEVICE_NAME" << "\" : \"" << dev._CL_DEVICE_NAME << "\"," << endl;
				cout << "\t\t\t\t\"" << "_CL_DEVICE_TYPE" << "\" : \"" << dev._CL_DEVICE_TYPE << "\"," << endl;
				cout << "\t\t\t\t\"" << "_CL_DEVICE_GLOBAL_MEM_SIZE" << "\" : " << dev._CL_DEVICE_GLOBAL_MEM_SIZE << "," << endl; // num
				cout << "\t\t\t\t\"" << "_CL_DEVICE_VENDOR" << "\" : \"" << dev._CL_DEVICE_VENDOR << "\"," << endl;
				cout << "\t\t\t\t\"" << "_CL_DEVICE_VERSION" << "\" : \"" << dev._CL_DEVICE_VERSION << "\"," << endl;
				cout << "\t\t\t\t\"" << "_CL_DRIVER_VERSION" << "\" : \"" << dev._CL_DRIVER_VERSION << "\"" << endl;
				cout << "\t\t\t}" << COMMA(devComma) << endl;
			}
			cout << "\t\t]" << endl;
			cout << "\t}" << COMMA(devPlatformsComma) << endl;
		}
	}

	cout << "]" << endl;
}


void AMDOpenCLDeviceDetection::PrintDevicesJsonDirty() {
	cout << "[";

	{
		int devPlatformsComma = _devicesPlatformsDevices.size();
		for (const auto &jsonLog : _devicesPlatformsDevices) {
			cout << "{";
			cout << "\"PlatformName\": \"" << jsonLog.PlatformName << "\"" << ",";
			cout << "\"PlatformNum\": " << jsonLog.PlatformNum << ",";
			cout << "\"Devices\" : [";
			// device print
			int devComma = jsonLog.Devices.size();
			for (const auto &dev : jsonLog.Devices) {
				cout << "{";
				cout << "\"" << "DeviceID" << "\" : " << dev.DeviceID << ","; // num
				cout << "\"" << "AMD_BUS_ID" << "\" : " << dev.AMD_BUS_ID << ","; // num
				cout << "\"" << "_CL_DEVICE_NAME" << "\" : \"" << dev._CL_DEVICE_NAME << "\",";
				cout << "\"" << "_CL_DEVICE_TYPE" << "\" : \"" << dev._CL_DEVICE_TYPE << "\",";
				cout << "\"" << "_CL_DEVICE_GLOBAL_MEM_SIZE" << "\" : " << dev._CL_DEVICE_GLOBAL_MEM_SIZE << ","; // num
				cout << "\"" << "_CL_DEVICE_VENDOR" << "\" : \"" << dev._CL_DEVICE_VENDOR << "\",";
				cout << "\"" << "_CL_DEVICE_VERSION" << "\" : \"" << dev._CL_DEVICE_VERSION << "\",";
				cout << "\"" << "_CL_DRIVER_VERSION" << "\" : \"" << dev._CL_DRIVER_VERSION << "\"";
				cout << "}" << COMMA(devComma);
			}
			cout << "]";
			cout << "}" << COMMA(devPlatformsComma);
		}
	}

	cout << "]" << endl;
}

//modify return value
string AMDOpenCLDeviceDetection::GetDevicesJson() {
	string devicesJson;
	/*cout << "[" << endl;*/
	devicesJson += "[\n";

	{
		int devPlatformsComma = _devicesPlatformsDevices.size();
		for (const auto &jsonLog : _devicesPlatformsDevices) {
			/*cout << "\t{" << endl;
			cout << "\t\t\"PlatformName\": \"" << jsonLog.PlatformName << "\"" << "," << endl;
			cout << "\t\t\"PlatformNum\": " << jsonLog.PlatformNum << "," << endl;
			cout << "\t\t\"Devices\" : [" << endl;*/
			devicesJson += "\t{\n";
			devicesJson += "\t\t\"PlatformName\": \"" + jsonLog.PlatformName + "\"" + ",\n";
			devicesJson += "\t\t\"PlatformNum\": " + to_string(jsonLog.PlatformNum) + ",\n";
			devicesJson += "\t\t\"Devices\" : [\n";

			// device print
			int devComma = jsonLog.Devices.size();
			for (const auto &dev : jsonLog.Devices) {
				//cout << "\t\t\t{" << endl;
				//cout << "\t\t\t\t\"" << "DeviceID" << "\" : " << dev.DeviceID << "," << endl; // num
				//cout << "\t\t\t\t\"" << "AMD_BUS_ID" << "\" : " << dev.AMD_BUS_ID << "," << endl; // num
				//cout << "\t\t\t\t\"" << "_CL_DEVICE_NAME" << "\" : \"" << dev._CL_DEVICE_NAME << "\"," << endl;
				//cout << "\t\t\t\t\"" << "_CL_DEVICE_TYPE" << "\" : \"" << dev._CL_DEVICE_TYPE << "\"," << endl;
				//cout << "\t\t\t\t\"" << "_CL_DEVICE_GLOBAL_MEM_SIZE" << "\" : " << dev._CL_DEVICE_GLOBAL_MEM_SIZE << "," << endl; // num
				//cout << "\t\t\t\t\"" << "_CL_DEVICE_VENDOR" << "\" : \"" << dev._CL_DEVICE_VENDOR << "\"," << endl;
				//cout << "\t\t\t\t\"" << "_CL_DEVICE_VERSION" << "\" : \"" << dev._CL_DEVICE_VERSION << "\"," << endl;
				//cout << "\t\t\t\t\"" << "_CL_DRIVER_VERSION" << "\" : \"" << dev._CL_DRIVER_VERSION << "\"" << endl;
				//cout << "\t\t\t}" << COMMA(devComma) << endl;
				devicesJson += "\t\t\t{\n";
				devicesJson += "\t\t\t\t\"DeviceID\" : " + to_string(dev.DeviceID) + ",\n";
				devicesJson += "\t\t\t\t\"AMD_BUS_ID\" : " + to_string(dev.AMD_BUS_ID) + ",\n";
				devicesJson += "\t\t\t\t\"_CL_DEVICE_NAME\" : \"" + dev._CL_DEVICE_NAME + "\",\n";
				devicesJson += "\t\t\t\t\"_CL_DEVICE_TYPE\" : \"" + dev._CL_DEVICE_TYPE + "\",\n";
				devicesJson += "\t\t\t\t\"_CL_DEVICE_GLOBAL_MEM_SIZE\" : " + to_string(dev._CL_DEVICE_GLOBAL_MEM_SIZE) + ",\n";
				devicesJson += "\t\t\t\t\"_CL_DEVICE_VENDOR\" : \"" + dev._CL_DEVICE_VENDOR + "\",\n";
				devicesJson += "\t\t\t\t\"_CL_DEVICE_VERSION\" : \"" + dev._CL_DEVICE_VERSION + "\",\n";
				devicesJson += "\t\t\t\t\"_CL_DRIVER_VERSION\" : \"" + dev._CL_DRIVER_VERSION + "\",\n";
				devicesJson += "\t\t\t}";
				devicesJson += COMMA(devComma);
				devicesJson += "\n";
			}
			/*cout << "\t\t]" << endl;
			cout << "\t}" << COMMA(devPlatformsComma) << endl;*/
			devicesJson += "\t\t]\n";
			devicesJson += "\t}";
			devicesJson += COMMA(devPlatformsComma);
			devicesJson += "\n";
		}
	}

	/*cout << "]" << endl;*/
	devicesJson += "]\n";
	//cout << devicesJson;
	return devicesJson;
}


string AMDOpenCLDeviceDetection::GetDevicesJsonDirty() {
	string devicesJson;
	//cout << "[";
	devicesJson += "[";
	{
		int devPlatformsComma = _devicesPlatformsDevices.size();
		for (const auto &jsonLog : _devicesPlatformsDevices) {
			/*cout << "{";
			cout << "\"PlatformName\": \"" << jsonLog.PlatformName << "\"" << ",";
			cout << "\"PlatformNum\": " << jsonLog.PlatformNum << ",";
			cout << "\"Devices\" : [";*/
			devicesJson += "{";
			devicesJson += "\"PlatformName\": \"" + jsonLog.PlatformName + "\"" + ",";
			devicesJson += "\"PlatformNum\": " + to_string(jsonLog.PlatformNum) + ",";
			devicesJson += "\"Devices\" : [";

			// device print
			int devComma = jsonLog.Devices.size();
			for (const auto &dev : jsonLog.Devices) {
				//cout << "{";
				//cout << "\"" << "DeviceID" << "\" : " << dev.DeviceID << ","; // num
				//cout << "\"" << "AMD_BUS_ID" << "\" : " << dev.AMD_BUS_ID << ","; // num
				//cout << "\"" << "_CL_DEVICE_NAME" << "\" : \"" << dev._CL_DEVICE_NAME << "\",";
				//cout << "\"" << "_CL_DEVICE_TYPE" << "\" : \"" << dev._CL_DEVICE_TYPE << "\",";
				//cout << "\"" << "_CL_DEVICE_GLOBAL_MEM_SIZE" << "\" : " << dev._CL_DEVICE_GLOBAL_MEM_SIZE << ","; // num
				//cout << "\"" << "_CL_DEVICE_VENDOR" << "\" : \"" << dev._CL_DEVICE_VENDOR << "\",";
				//cout << "\"" << "_CL_DEVICE_VERSION" << "\" : \"" << dev._CL_DEVICE_VERSION << "\",";
				//cout << "\"" << "_CL_DRIVER_VERSION" << "\" : \"" << dev._CL_DRIVER_VERSION << "\"";
				//cout << "}" << COMMA(devComma);
				devicesJson += "{";
				devicesJson += "\"DeviceID\" : " + to_string(dev.DeviceID) + ","; // num
				devicesJson += "\"AMD_BUS_ID\" : " + to_string(dev.AMD_BUS_ID) + ","; // num
				devicesJson += "\"_CL_DEVICE_NAME\" : \"" + dev._CL_DEVICE_NAME + "\",";
				devicesJson += "\"_CL_DEVICE_TYPE\" : \"" + dev._CL_DEVICE_TYPE + "\",";
				devicesJson += "\"_CL_DEVICE_GLOBAL_MEM_SIZE\" : " + to_string(dev._CL_DEVICE_GLOBAL_MEM_SIZE) + ","; // num
				devicesJson += "\"_CL_DEVICE_VENDOR\" : \"" + dev._CL_DEVICE_VENDOR + "\",";
				devicesJson += "\"_CL_DEVICE_VERSION\" : \"" + dev._CL_DEVICE_VERSION + "\",";
				devicesJson += "\"_CL_DRIVER_VERSION\" : \"" + dev._CL_DRIVER_VERSION + "\"";
				devicesJson += "}";
				devicesJson += COMMA(devComma);
				//devicesJson += "\n";
			}
			/*cout << "]";
			cout << "}" << COMMA(devPlatformsComma);*/
			devicesJson += "]";
			devicesJson += "}";
			devicesJson += COMMA(devPlatformsComma);
			//devicesJson += "\n";
		}
	}

	/*cout << "]" << endl;*/
	devicesJson += "]\n";
	return  devicesJson;
}

extern "C"
{
	__declspec(dllexport) bool  _QueryDevices()
	{
		AMDOpenCLDeviceDetection amd;
		return amd.QueryDevices();
	}
	//__declspec(dllexport) string  _GetDevicesJsonDirty()
	//{
	//	AMDOpenCLDeviceDetection amd;
	//	//const char* p = amd.GetDevicesJsonDirty().data();
	//	return amd.GetDevicesJsonDirty();
	//}
	__declspec(dllexport) char* __cdecl _GetDevicesJson()
	{
		AMDOpenCLDeviceDetection amd;
		static string deviceJson;
		if (amd.QueryDevices()) {
			//const char* p = amd.GetDevicesJsonDirty().data();
			deviceJson = amd.GetDevicesJson();
		}
		char *charJson = (char*)deviceJson.c_str();
		return charJson;
	}
	__declspec(dllexport) char* __cdecl _GetDevicesJsonDirty()
	{
		AMDOpenCLDeviceDetection amd;
		static string deviceJson;
		if (amd.QueryDevices()) {
			deviceJson = amd.GetDevicesJsonDirty();
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