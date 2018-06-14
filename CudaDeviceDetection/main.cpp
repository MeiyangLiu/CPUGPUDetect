//#include "CudaDetection.h"
//#include <iostream>
//
//// TODO maybe add nvml.dll check
////#include <Windows.h>
//
//int main(int argc, char* argv[]) {
//	CudaDetection detection;
//	if (detection.QueryDevices()) {
//		if (argc < 2) {
//			//detection.PrintDevicesJson_d();
//			std::string json = detection.GetDevicesJson_d();
//			std::cout << "json from the GetDevicesJson_d founction modified by liumy:" << std::endl;
//			std::cout << json;// << std::endl;
//		}
//		else {
//			//detection.PrintDevicesJson();
//			std::string json = detection.GetDevicesJson();
//			std::cout << "json from the GetDevicesJson founction modified by liumy:" << std::endl;
//			std::cout << json;// << std::endl;
//		}
//	}
//	return 0;
//}
