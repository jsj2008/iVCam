#ifndef BLENDER_WRAPPER_H
#define BLENDER_WRAPPER_H

#ifdef _WIN
#define EXPORT_API __declspec(dllexport)
#else
#define EXPORT_API
#endif

#include <string>
#include <mutex>
#include <atomic>

typedef struct _BlenderParams {
	unsigned int	input_width		= 0;
	unsigned int	input_height	= 0;
	unsigned int	output_width	= 0;
	unsigned int	output_height	= 0;
	unsigned char*	input_data		= nullptr;
	unsigned char*	output_data		= nullptr; 
	std::string		offset			= "";
}BlenderParams, *BlenderParamsPtr;

class CBaseBlender;
class Timer;

class EXPORT_API CBlenderWrapper
{
public:
	enum BLENDER_DEVICE_TYPE {
		CUDA_BLENDER,
		OPENCL_BLENDER,
		CPU_BLENDER
	};
 
	enum BLENDER_TYPE {
		PANORAMIC_BLENDER			= 1,
		THREEDIMENSION_BLENDER		= 2,
		PANORAMIC_CYLINDER_BLENDER	= 3,
		THREEDIMENSION_TWOLENS_BLENDER = 4
	};
	enum COLOR_MODE {
		THREE_CHANNELS    = 1,
		FOUR_CHANNELS     = 2,
		THREE_IN_FOUR_OUT = 3,
		FOUR_IN_THREE_OUT = 4
	};

	explicit CBlenderWrapper();
	~CBlenderWrapper();

public:
	int capabilityAssessment();
	void getSingleInstance(COLOR_MODE mode);
	bool initializeDevice();
	bool runImageBlender(BlenderParams& params, BLENDER_TYPE type); 
	void runImageBlenderComp(unsigned int input_width, unsigned int input_height, unsigned int output_width, unsigned int output_height, unsigned char* input_data, unsigned char* output_data, char* offset, BLENDER_TYPE type);
private:
	bool checkParameters(BlenderParams& params);
	bool isSupportCUDA();
	bool isSupportOpenCL();

private: 
	std::string m_offset;
	BLENDER_DEVICE_TYPE m_deviceType;
	std::mutex m_mutex;
	std::atomic<CBaseBlender*> m_blender;
};

#endif




