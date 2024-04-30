// Copyright (c) 2024-2024, A3
//
// License: MIT

#include <iostream>
#include <fstream>
#include <string>
#include <chrono>
#include <windows.h>

#include <openxr/openxr.h>

#include <osc\OscOutboundPacketStream.h>
#include <ip\UdpSocket.h>

#define CHK_XR(r,e) if(XR_FAILED(r)) { std::cout << e << ":" << r << "\n";return -1;}
const float PI = 3.14159265358979;

XrVector3f euler(XrQuaternionf q) {
	auto sy = 2 * q.x * q.z + 2 * q.y * q.w;
	auto unlocked = std::abs(sy) < 0.99999f;
	return XrVector3f{
		unlocked ? std::atan2(-(2 * q.y * q.z - 2 * q.x * q.w), 2 * q.w * q.w + 2 * q.z * q.z - 1)
		: std::atan2(2 * q.y * q.z + 2 * q.x * q.w, 2 * q.w * q.w + 2 * q.y * q.y - 1),
		std::asin(sy),
		unlocked ? std::atan2(-(2 * q.x * q.y - 2 * q.z * q.w), 2 * q.w * q.w + 2 * q.x * q.x - 1) : 0
	};
}
XrQuaternionf invert(XrQuaternionf q) {
	XrQuaternionf result;
	result.x = -q.x;
	result.y = -q.y;
	result.z = -q.z;
	result.w = q.w;
	return result;
}

XrQuaternionf operator*(XrQuaternionf a, XrQuaternionf b) {
	XrQuaternionf result;
	result.x = (b.w * a.x) + (b.x * a.w) + (b.y * a.z) - (b.z * a.y);
	result.y = (b.w * a.y) - (b.x * a.z) + (b.y * a.w) + (b.z * a.x);
	result.z = (b.w * a.z) + (b.x * a.y) - (b.y * a.x) + (b.z * a.w);
	result.w = (b.w * a.w) - (b.x * a.x) - (b.y * a.y) - (b.z * a.z);
	return result;
}
float InverseLerp(float a, float b, float value) {
	return (value - a) / (b - a);
}
const int UNITY_FINGER_COUNT = 40;

XrHandJointEXT FingerNameStretched_OpenXR[] =
{
	XR_HAND_JOINT_THUMB_METACARPAL_EXT,
	XR_HAND_JOINT_THUMB_PROXIMAL_EXT,
	XR_HAND_JOINT_THUMB_DISTAL_EXT,
	XR_HAND_JOINT_INDEX_PROXIMAL_EXT,
	XR_HAND_JOINT_INDEX_INTERMEDIATE_EXT,
	XR_HAND_JOINT_INDEX_DISTAL_EXT,
	XR_HAND_JOINT_MIDDLE_PROXIMAL_EXT,
	XR_HAND_JOINT_MIDDLE_INTERMEDIATE_EXT,
	XR_HAND_JOINT_MIDDLE_DISTAL_EXT,
	XR_HAND_JOINT_RING_PROXIMAL_EXT,
	XR_HAND_JOINT_RING_INTERMEDIATE_EXT,
	XR_HAND_JOINT_RING_DISTAL_EXT,
	XR_HAND_JOINT_LITTLE_PROXIMAL_EXT,
	XR_HAND_JOINT_LITTLE_INTERMEDIATE_EXT,
	XR_HAND_JOINT_LITTLE_DISTAL_EXT,
};
XrHandJointEXT FingerNameSpread_OpenXR[] =
{
	XR_HAND_JOINT_THUMB_PROXIMAL_EXT,
	XR_HAND_JOINT_INDEX_PROXIMAL_EXT,
	XR_HAND_JOINT_MIDDLE_PROXIMAL_EXT,
	XR_HAND_JOINT_RING_PROXIMAL_EXT,
	XR_HAND_JOINT_LITTLE_PROXIMAL_EXT,
};
XrHandJointEXT FingerNameStretched_OpenXR_Oya[] =
{
	XR_HAND_JOINT_WRIST_EXT,
	XR_HAND_JOINT_THUMB_METACARPAL_EXT,
	XR_HAND_JOINT_THUMB_PROXIMAL_EXT,
	XR_HAND_JOINT_INDEX_METACARPAL_EXT,
	XR_HAND_JOINT_INDEX_PROXIMAL_EXT,
	XR_HAND_JOINT_INDEX_INTERMEDIATE_EXT,
	XR_HAND_JOINT_MIDDLE_METACARPAL_EXT,
	XR_HAND_JOINT_MIDDLE_PROXIMAL_EXT,
	XR_HAND_JOINT_MIDDLE_INTERMEDIATE_EXT,
	XR_HAND_JOINT_RING_METACARPAL_EXT,
	XR_HAND_JOINT_RING_PROXIMAL_EXT,
	XR_HAND_JOINT_RING_INTERMEDIATE_EXT,
	XR_HAND_JOINT_LITTLE_METACARPAL_EXT,
	XR_HAND_JOINT_LITTLE_PROXIMAL_EXT,
	XR_HAND_JOINT_LITTLE_INTERMEDIATE_EXT,
};
XrHandJointEXT FingerNameSpread_OpenXR_Oya[] =
{
	XR_HAND_JOINT_WRIST_EXT,
	XR_HAND_JOINT_THUMB_METACARPAL_EXT,
	XR_HAND_JOINT_MIDDLE_METACARPAL_EXT,
	XR_HAND_JOINT_RING_METACARPAL_EXT,
	XR_HAND_JOINT_LITTLE_METACARPAL_EXT,
};

const char* pathFull[] = {
	"/avatar/parameters/LHThumb1",
	"/avatar/parameters/LHThumb2",
	"/avatar/parameters/LHThumb3",
	"/avatar/parameters/LHIndex1",
	"/avatar/parameters/LHIndex2",
	"/avatar/parameters/LHIndex3",
	"/avatar/parameters/LHMiddle1",
	"/avatar/parameters/LHMiddle2",
	"/avatar/parameters/LHMiddle3",
	"/avatar/parameters/LHRing1",
	"/avatar/parameters/LHRing2",
	"/avatar/parameters/LHRing3",
	"/avatar/parameters/LHLittle1",
	"/avatar/parameters/LHLittle2",
	"/avatar/parameters/LHLittle3",
	"/avatar/parameters/RHThumb1",
	"/avatar/parameters/RHThumb2",
	"/avatar/parameters/RHThumb3",
	"/avatar/parameters/RHIndex1",
	"/avatar/parameters/RHIndex2",
	"/avatar/parameters/RHIndex3",
	"/avatar/parameters/RHMiddle1",
	"/avatar/parameters/RHMiddle2",
	"/avatar/parameters/RHMiddle3",
	"/avatar/parameters/RHRing1",
	"/avatar/parameters/RHRing2",
	"/avatar/parameters/RHRing3",
	"/avatar/parameters/RHLittle1",
	"/avatar/parameters/RHLittle2",
	"/avatar/parameters/RHLittle3",
	"/avatar/parameters/LHThumbS",
	"/avatar/parameters/LHIndexS",
	"/avatar/parameters/LHMiddleS",
	"/avatar/parameters/LHRingS",
	"/avatar/parameters/LHLittleS",
	"/avatar/parameters/RHThumbS",
	"/avatar/parameters/RHIndexS",
	"/avatar/parameters/RHMiddleS",
	"/avatar/parameters/RHRingS",
	"/avatar/parameters/RHLittleS",
};

const char* pathEco[] = {
	"/avatar/parameters/Thumb1",
	"/avatar/parameters/Thumb2",
	"/avatar/parameters/Thumb3",
	"/avatar/parameters/Index1",
	"/avatar/parameters/Index2",
	"/avatar/parameters/Index3",
	"/avatar/parameters/Middle1",
	"/avatar/parameters/Middle2",
	"/avatar/parameters/Middle3",
	"/avatar/parameters/Ring1",
	"/avatar/parameters/Ring2",
	"/avatar/parameters/Ring3",
	"/avatar/parameters/Little1",
	"/avatar/parameters/Little2",
	"/avatar/parameters/Little3",
	"/avatar/parameters/Thumb1",
	"/avatar/parameters/Thumb2",
	"/avatar/parameters/Thumb3",
	"/avatar/parameters/Index1",
	"/avatar/parameters/Index2",
	"/avatar/parameters/Index3",
	"/avatar/parameters/Middle1",
	"/avatar/parameters/Middle2",
	"/avatar/parameters/Middle3",
	"/avatar/parameters/Ring1",
	"/avatar/parameters/Ring2",
	"/avatar/parameters/Ring3",
	"/avatar/parameters/Little1",
	"/avatar/parameters/Little2",
	"/avatar/parameters/Little3",
	"/avatar/parameters/ThumbS",
	"/avatar/parameters/IndexS",
	"/avatar/parameters/MiddleS",
	"/avatar/parameters/RingS",
	"/avatar/parameters/LittleS",
	"/avatar/parameters/ThumbS",
	"/avatar/parameters/IndexS",
	"/avatar/parameters/MiddleS",
	"/avatar/parameters/RingS",
	"/avatar/parameters/LittleS",
};

int main(int argc, char** argv)
{
	std::cout << "Finger Tracker OSC\n(c)A3\nhttps://twitter.com/A3_yuu\n";

	//arg
	const char *ip = "127.0.0.1";
	int port = 9000;
	int sleepTime = 33;
	int mode = 2;

	switch (argc > 5 ? 5 : argc) {
	case 5:
		mode = atoi(argv[4]);
	case 4:
		sleepTime = atoi(argv[3]);
	case 3:
		port = atoi(argv[2]);
	case 2:
		ip = argv[1];
	default:
		break;
	}

	//file
	float muscleDegree[UNITY_FINGER_COUNT];
	float muscleDefaultMin[UNITY_FINGER_COUNT];
	float muscleDefaultMax[UNITY_FINGER_COUNT];
	{
		std::ifstream ifs("num.txt");
		for (int i = 0; i < UNITY_FINGER_COUNT; i++) {
			std::string str;
			if (!std::getline(ifs, str)) {
				std::cout << "No num.txt";
				return -1;
			}
			muscleDegree[i] = atof(str.c_str());
		}
	}
	{
		std::ifstream ifs("muscle.txt");
		for (int i = 0; i < UNITY_FINGER_COUNT; i++) {
			std::string str;
			if (!std::getline(ifs, str)) {
				std::cout << "No muscle.txt";
				return -1;
			}
			muscleDefaultMin[i] = atof(str.c_str());
			if (!std::getline(ifs, str)) {
				std::cout << "No muscle.txt";
				return -1;
			}
			muscleDefaultMax[i] = atof(str.c_str());
		}
	}

	//OpenXR
	//Create instance
	XrApplicationInfo applicationInfo{
		"A3 Finger Tracker",
		1,//app ver
		"",
		0,
		XR_CURRENT_API_VERSION
	};
	const int extensionsCount = 2;
	const char* extensions[] = {
	  "XR_EXT_hand_tracking",
	  "XR_MND_headless",
	};
	XrInstanceCreateInfo instanceCreateInfo = { XR_TYPE_INSTANCE_CREATE_INFO };
	instanceCreateInfo.next = NULL;
	instanceCreateInfo.createFlags = 0;
	instanceCreateInfo.applicationInfo = applicationInfo;
	instanceCreateInfo.enabledApiLayerCount = 0;
	instanceCreateInfo.enabledApiLayerNames = NULL;
	instanceCreateInfo.enabledExtensionCount = extensionsCount;
	instanceCreateInfo.enabledExtensionNames = extensions;

	XrInstance instance;
	CHK_XR(xrCreateInstance(&instanceCreateInfo, &instance), "xrCreateInstance");

	//Get systemId
	XrSystemGetInfo systemGetInfo = { XR_TYPE_SYSTEM_GET_INFO };
	systemGetInfo.next = NULL;
	systemGetInfo.formFactor = XR_FORM_FACTOR_HEAD_MOUNTED_DISPLAY;

	XrSystemId systemId;
	CHK_XR(xrGetSystem(instance, &systemGetInfo, &systemId), "xrGetSystem");

	//Create session
	XrSessionCreateInfo sessionCreateInfo = { XR_TYPE_SESSION_CREATE_INFO };
	sessionCreateInfo.next = NULL;
	sessionCreateInfo.createFlags = 0;
	sessionCreateInfo.systemId = systemId;

	XrSession session;
	CHK_XR(xrCreateSession(instance, &sessionCreateInfo, &session), "xrCreateSession");

	//XrSpace
	XrReferenceSpaceCreateInfo referenceSpaceCreateInfo{ XR_TYPE_REFERENCE_SPACE_CREATE_INFO };
	XrPosef t{};
	t.orientation.w = 1;
	referenceSpaceCreateInfo.poseInReferenceSpace = t;
	referenceSpaceCreateInfo.referenceSpaceType = XR_REFERENCE_SPACE_TYPE_VIEW;
	XrSpace xrSpace;
	xrCreateReferenceSpace(session, &referenceSpaceCreateInfo, &xrSpace);

	//Hand Tracking
	XrSystemHandTrackingPropertiesEXT handTrackingSystemProperties{ XR_TYPE_SYSTEM_HAND_TRACKING_PROPERTIES_EXT };
	XrSystemProperties systemProperties{ XR_TYPE_SYSTEM_PROPERTIES,&handTrackingSystemProperties };
	CHK_XR(xrGetSystemProperties(instance, systemId, &systemProperties), "xrGetSystemProperties");
	if (!handTrackingSystemProperties.supportsHandTracking) {
		return -1;
	}
	PFN_xrCreateHandTrackerEXT pfnCreateHandTrackerEXT;
	CHK_XR(xrGetInstanceProcAddr(instance, "xrCreateHandTrackerEXT", reinterpret_cast<PFN_xrVoidFunction*>(&pfnCreateHandTrackerEXT)), "xrGetInstanceProcAddr");
	PFN_xrLocateHandJointsEXT pfnLocateHandJointsEXT;
	CHK_XR(xrGetInstanceProcAddr(instance, "xrLocateHandJointsEXT", reinterpret_cast<PFN_xrVoidFunction*>(&pfnLocateHandJointsEXT)), "xrGetInstanceProcAddr");

	//LeftData
	XrHandTrackerEXT handTrackerLeft{};
	{
		XrHandTrackerCreateInfoEXT createInfo{ XR_TYPE_HAND_TRACKER_CREATE_INFO_EXT };
		createInfo.hand = XR_HAND_LEFT_EXT;
		createInfo.handJointSet = XR_HAND_JOINT_SET_DEFAULT_EXT;
		CHK_XR(pfnCreateHandTrackerEXT(session, &createInfo, &handTrackerLeft), "pfnCreateHandTrackerEXT");
	}
	XrHandJointLocationEXT jointLocationsLeft[XR_HAND_JOINT_COUNT_EXT];
	XrHandJointVelocityEXT jointVelocitiesLeft[XR_HAND_JOINT_COUNT_EXT];
	XrHandJointVelocitiesEXT velocitiesLeft{ XR_TYPE_HAND_JOINT_VELOCITIES_EXT };
	velocitiesLeft.jointCount = XR_HAND_JOINT_COUNT_EXT;
	velocitiesLeft.jointVelocities = jointVelocitiesLeft;
	XrHandJointLocationsEXT locationsLeft{ XR_TYPE_HAND_JOINT_LOCATIONS_EXT };
	locationsLeft.next = &velocitiesLeft;
	locationsLeft.jointCount = XR_HAND_JOINT_COUNT_EXT;
	locationsLeft.jointLocations = jointLocationsLeft;

	//RightData
	XrHandTrackerEXT handTrackerRight{};
	{
		XrHandTrackerCreateInfoEXT createInfo{ XR_TYPE_HAND_TRACKER_CREATE_INFO_EXT };
		createInfo.hand = XR_HAND_RIGHT_EXT;
		createInfo.handJointSet = XR_HAND_JOINT_SET_DEFAULT_EXT;
		CHK_XR(pfnCreateHandTrackerEXT(session, &createInfo, &handTrackerRight), "pfnCreateHandTrackerEXT");
	}
	XrHandJointLocationEXT jointLocationsRight[XR_HAND_JOINT_COUNT_EXT];
	XrHandJointVelocityEXT jointVelocitiesRight[XR_HAND_JOINT_COUNT_EXT];
	XrHandJointVelocitiesEXT velocitiesRight{ XR_TYPE_HAND_JOINT_VELOCITIES_EXT };
	velocitiesRight.jointCount = XR_HAND_JOINT_COUNT_EXT;
	velocitiesRight.jointVelocities = jointVelocitiesRight;
	XrHandJointLocationsEXT locationsRight{ XR_TYPE_HAND_JOINT_LOCATIONS_EXT };
	locationsRight.next = &velocitiesRight;
	locationsRight.jointCount = XR_HAND_JOINT_COUNT_EXT;
	locationsRight.jointLocations = jointLocationsRight;

	//OSC
	UdpTransmitSocket transmitSocket(IpEndpointName(ip, port));

	std::cout << "OK!" << std::endl;
	//Loop
	int c = 0;
	while (1) {
		Sleep(sleepTime);
		int64_t time = std::chrono::duration_cast<std::chrono::nanoseconds>(std::chrono::system_clock::now().time_since_epoch()).count();

		//Hand Tracking
		XrHandJointsLocateInfoEXT locateInfo{ XR_TYPE_HAND_JOINTS_LOCATE_INFO_EXT };
		locateInfo.baseSpace = xrSpace;
		locateInfo.time = time;

		CHK_XR(pfnLocateHandJointsEXT(handTrackerLeft, &locateInfo, &locationsLeft),"pfnLocateHandJointsEXT");
		CHK_XR(pfnLocateHandJointsEXT(handTrackerRight, &locateInfo, &locationsRight), "pfnLocateHandJointsEXT");
		
		if (!locationsLeft.isActive || !locationsRight.isActive) continue;
		float data[UNITY_FINGER_COUNT];

		//Get Rotations
		XrQuaternionf boneRotations[UNITY_FINGER_COUNT];
		for (int i = 0; i < 15; i++)
		{
			XrQuaternionf pose;
			XrQuaternionf poseoya;
			pose = jointLocationsLeft[FingerNameStretched_OpenXR[i]].pose.orientation;
			poseoya = jointLocationsLeft[FingerNameStretched_OpenXR_Oya[i]].pose.orientation;
			boneRotations[i] = invert(poseoya) * pose;
			pose = jointLocationsRight[FingerNameStretched_OpenXR[i]].pose.orientation;
			poseoya = jointLocationsRight[FingerNameStretched_OpenXR_Oya[i]].pose.orientation;
			boneRotations[i + 15] = invert(poseoya) * pose;
		}
		for (int i = 0; i < 5; i++)
		{
			XrQuaternionf pose;
			XrQuaternionf poseoya;
			pose = jointLocationsLeft[FingerNameSpread_OpenXR[i]].pose.orientation;
			poseoya = jointLocationsLeft[FingerNameSpread_OpenXR_Oya[i]].pose.orientation;
			boneRotations[i + 30] = invert(poseoya) * pose;
			pose = jointLocationsRight[FingerNameSpread_OpenXR[i]].pose.orientation;
			poseoya = jointLocationsRight[FingerNameSpread_OpenXR_Oya[i]].pose.orientation;
			boneRotations[i + 35] = invert(poseoya) * pose;
		}

		float eulerAngles[UNITY_FINGER_COUNT];
		//Get Stretched
		for (int i = 0; i < 30; i++)
		{
			eulerAngles[i] = -euler(boneRotations[i]).x;
			eulerAngles[i] += muscleDegree[i];
			if (eulerAngles[i] > 180) eulerAngles[i] -= 360;
			if (eulerAngles[i] < -180) eulerAngles[i] += 360;
			data[i] = InverseLerp(muscleDefaultMin[i], muscleDefaultMax[i], eulerAngles[i]) * 2 - 1;
		}

		//Get Spread
		eulerAngles[30] = -euler(boneRotations[30]).y;
		eulerAngles[31] = euler(boneRotations[31]).y;
		eulerAngles[32] = euler(boneRotations[32]).y;
		eulerAngles[33] = -euler(boneRotations[33]).y;
		eulerAngles[34] = -euler(boneRotations[34]).y;
		eulerAngles[35] = euler(boneRotations[35]).y;
		eulerAngles[36] = -euler(boneRotations[36]).y;
		eulerAngles[37] = -euler(boneRotations[37]).y;
		eulerAngles[38] = euler(boneRotations[38]).y;
		eulerAngles[39] = euler(boneRotations[39]).y;
		for (int i = 30; i < 40; i++)
		{
			eulerAngles[i] += muscleDegree[i];
			if (eulerAngles[i] > 180) eulerAngles[i] -= 360;
			if (eulerAngles[i] < -180) eulerAngles[i] += 360;
			data[i] = InverseLerp(muscleDefaultMin[i], muscleDefaultMax[i], eulerAngles[i]) * 2 - 1;
		}

		//OSC
		char buffer[6144];
		osc::OutboundPacketStream p(buffer, 6144);
		p << osc::BeginBundleImmediate;
		switch (mode)
		{
			//Economy:1fps from others. Time divide 320bit into 80bit (128bit limit era)
		case 1:
			c++;
			p << osc::BeginMessage("/avatar/parameters/FingerFlag") << 0 << osc::EndMessage;
			switch (c % 4)
			{
			case 0:
				p << osc::BeginMessage("/avatar/parameters/Finger0") << data[0] << osc::EndMessage;
				p << osc::BeginMessage("/avatar/parameters/Finger1") << data[1] << osc::EndMessage;
				p << osc::BeginMessage("/avatar/parameters/Finger2") << data[2] << osc::EndMessage;
				p << osc::BeginMessage("/avatar/parameters/Finger3") << data[30] << osc::EndMessage;
				p << osc::BeginMessage("/avatar/parameters/Finger4") << data[3] << osc::EndMessage;
				p << osc::BeginMessage("/avatar/parameters/Finger5") << data[4] << osc::EndMessage;
				p << osc::BeginMessage("/avatar/parameters/Finger6") << data[5] << osc::EndMessage;
				p << osc::BeginMessage("/avatar/parameters/Finger7") << data[31] << osc::EndMessage;
				p << osc::BeginMessage("/avatar/parameters/Finger8") << data[6] << osc::EndMessage;
				p << osc::BeginMessage("/avatar/parameters/Finger9") << data[7] << osc::EndMessage;
				break;
			case 1:
				p << osc::BeginMessage("/avatar/parameters/Finger0") << data[8] << osc::EndMessage;
				p << osc::BeginMessage("/avatar/parameters/Finger1") << data[32] << osc::EndMessage;
				p << osc::BeginMessage("/avatar/parameters/Finger2") << data[9] << osc::EndMessage;
				p << osc::BeginMessage("/avatar/parameters/Finger3") << data[10] << osc::EndMessage;
				p << osc::BeginMessage("/avatar/parameters/Finger4") << data[11] << osc::EndMessage;
				p << osc::BeginMessage("/avatar/parameters/Finger5") << data[33] << osc::EndMessage;
				p << osc::BeginMessage("/avatar/parameters/Finger6") << data[12] << osc::EndMessage;
				p << osc::BeginMessage("/avatar/parameters/Finger7") << data[13] << osc::EndMessage;
				p << osc::BeginMessage("/avatar/parameters/Finger8") << data[14] << osc::EndMessage;
				p << osc::BeginMessage("/avatar/parameters/Finger9") << data[34] << osc::EndMessage;
				break;
			case 2:
				p << osc::BeginMessage("/avatar/parameters/Finger0") << data[15] << osc::EndMessage;
				p << osc::BeginMessage("/avatar/parameters/Finger1") << data[16] << osc::EndMessage;
				p << osc::BeginMessage("/avatar/parameters/Finger2") << data[17] << osc::EndMessage;
				p << osc::BeginMessage("/avatar/parameters/Finger3") << data[35] << osc::EndMessage;
				p << osc::BeginMessage("/avatar/parameters/Finger4") << data[18] << osc::EndMessage;
				p << osc::BeginMessage("/avatar/parameters/Finger5") << data[19] << osc::EndMessage;
				p << osc::BeginMessage("/avatar/parameters/Finger6") << data[20] << osc::EndMessage;
				p << osc::BeginMessage("/avatar/parameters/Finger7") << data[36] << osc::EndMessage;
				p << osc::BeginMessage("/avatar/parameters/Finger8") << data[21] << osc::EndMessage;
				p << osc::BeginMessage("/avatar/parameters/Finger9") << data[22] << osc::EndMessage;
				break;
			case 3:
				p << osc::BeginMessage("/avatar/parameters/Finger0") << data[23] << osc::EndMessage;
				p << osc::BeginMessage("/avatar/parameters/Finger1") << data[37] << osc::EndMessage;
				p << osc::BeginMessage("/avatar/parameters/Finger2") << data[24] << osc::EndMessage;
				p << osc::BeginMessage("/avatar/parameters/Finger3") << data[25] << osc::EndMessage;
				p << osc::BeginMessage("/avatar/parameters/Finger4") << data[26] << osc::EndMessage;
				p << osc::BeginMessage("/avatar/parameters/Finger5") << data[38] << osc::EndMessage;
				p << osc::BeginMessage("/avatar/parameters/Finger6") << data[27] << osc::EndMessage;
				p << osc::BeginMessage("/avatar/parameters/Finger7") << data[28] << osc::EndMessage;
				p << osc::BeginMessage("/avatar/parameters/Finger8") << data[29] << osc::EndMessage;
				p << osc::BeginMessage("/avatar/parameters/Finger9") << data[39] << osc::EndMessage;
				break;
			}
			p << osc::BeginMessage("/avatar/parameters/FingerFlag") << c % 4 + 1 << osc::EndMessage;
			break;
			//Economy:1fps from others. Time divide 320bit into 160bit by LR
		case 2:
			c++;
			p << osc::BeginMessage("/avatar/parameters/FingerFlag") << 0 << osc::EndMessage;
			switch (c % 2)
			{
			case 0://L
				for (int i = 0; i < 15; i++)
				{
					p << osc::BeginMessage(pathEco[i]) << data[i] << osc::EndMessage;
				}
				for (int i = 30; i < 35; i++)
				{
					p << osc::BeginMessage(pathEco[i]) << data[i] << osc::EndMessage;
				}
				break;
			case 1://R
				for (int i = 15; i < 30; i++)
				{
					p << osc::BeginMessage(pathEco[i]) << data[i] << osc::EndMessage;
				}
				for (int i = 35; i < 40; i++)
				{
					p << osc::BeginMessage(pathEco[i]) << data[i] << osc::EndMessage;
				}
				break;
			}
			p << osc::BeginMessage("/avatar/parameters/FingerFlag") << c % 2 + 1 << osc::EndMessage;
			break;
			//FullData:Moves smoothly. 320bit but cut off one finger on VRC(248bit)
		default:
			for (int i = 0; i < 40; i++)
			{
				p << osc::BeginMessage(pathFull[i]) << data[i] << osc::EndMessage;
			}
			break;
		}
		p << osc::EndBundle;
		transmitSocket.Send(p.Data(), p.Size());
	}

	return 0;
}