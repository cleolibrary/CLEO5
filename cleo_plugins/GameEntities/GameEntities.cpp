#include "plugin.h"
#include "CLEO.h"
#include "CLEO_Utils.h"

using namespace CLEO;
using namespace plugin;
using namespace std;

class GameEntities
{
public:
	GameEntities()
	{
		auto cleoVer = CLEO_GetVersion();
		if (cleoVer < CLEO_VERSION)
		{
			auto err = StringPrintf("This plugin requires version %X or later! \nCurrent version of CLEO is %X.", CLEO_VERSION >> 8, cleoVer >> 8);
			MessageBox(HWND_DESKTOP, err.c_str(), TARGET_NAME, MB_SYSTEMMODAL | MB_ICONERROR);
			return;
		}

		// register opcodes
		CLEO_RegisterOpcode(0x0AB5, opcode_0AB5); // store_closest_entities
	}

	//0AB5=3,store_closest_entities {char} %1d% {carHandle} %2d% {charHandle} %3d%
	static OpcodeResult __stdcall opcode_0AB5(CRunningScript* thread)
	{
		auto pedHandle = OPCODE_READ_PARAM_PED_HANDLE();

		auto ped = CPools::GetPed(pedHandle);
		if (ped == nullptr || ped->m_pIntelligence == nullptr)
		{
			OPCODE_WRITE_PARAM_INT(-1);
			OPCODE_WRITE_PARAM_INT(-1);
			return OR_CONTINUE;
		}

		DWORD foundCar = -1;
		const auto& cars = ped->m_pIntelligence->m_vehicleScanner.m_apEntities;
		for (size_t i = 0; i < std::size(cars); i++)
		{
			auto car = (CVehicle*)cars[i];
			if (car != nullptr &&
				car->m_nCreatedBy != eVehicleCreatedBy::MISSION_VEHICLE &&
				!car->m_nVehicleFlags.bFadeOut)
			{
				foundCar = CPools::GetVehicleRef(car);
				break;
			}
		}

		DWORD foundPed = -1;
		const auto& peds = ped->m_pIntelligence->m_pedScanner.m_apEntities;
		for (size_t i = 0; i < std::size(cars); i++)
		{
			auto ped = (CPed*)peds[i];
			if (ped != nullptr &&
				ped->m_nCreatedBy == 1 && // random pedestrian
				!ped->m_nPedFlags.bFadeOut)
			{
				foundPed = CPools::GetPedRef(ped);
				break;
			}
		}

		OPCODE_WRITE_PARAM_INT(foundCar);
		OPCODE_WRITE_PARAM_INT(foundPed);
		return OR_CONTINUE;
	}
} Instance;
