#include "plugin.h"
#include "CLEO.h"
#include "CLEO_Utils.h"
#include "CMenuManager.h"
#include "CRadar.h"
#include "CWorld.h"

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
		CLEO_RegisterOpcode(0x0AB6, opcode_0AB6); // get_target_blip_coords
		CLEO_RegisterOpcode(0x0AB7, opcode_0AB7); // get_car_number_of_gears
		CLEO_RegisterOpcode(0x0AB8, opcode_0AB8); // get_car_current_gear
		CLEO_RegisterOpcode(0x0ABD, opcode_0ABD); // is_car_siren_on
		CLEO_RegisterOpcode(0x0ABE, opcode_0ABE); // is_car_engine_on
		CLEO_RegisterOpcode(0x0ABF, opcode_0ABF); // cleo_set_car_engine_on
	}

	// store_closest_entities
	// [var carHandle: Car], [var charHandle: Char] = store_closest_entities [Char]
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
				foundCar = CPools::GetVehicleRef(car); // get handle
				break;
			}
		}

		DWORD foundPed = -1;
		const auto& peds = ped->m_pIntelligence->m_pedScanner.m_apEntities;
		for (size_t i = 0; i < std::size(peds); i++)
		{
			auto ped = (CPed*)peds[i];
			if (ped != nullptr &&
				ped->m_nCreatedBy == 1 && // random pedestrian
				!ped->m_nPedFlags.bFadeOut)
			{
				foundPed = CPools::GetPedRef(ped); // get handle
				break;
			}
		}

		OPCODE_WRITE_PARAM_INT(foundCar);
		OPCODE_WRITE_PARAM_INT(foundPed);
		return OR_CONTINUE;
	}

	// get_target_blip_coords
	// [var x: float], [var y: float], [var z: float] = get_target_blip_coords (logical)
	static OpcodeResult __stdcall opcode_0AB6(CRunningScript* thread)
	{
		auto blipIdx = CRadar::GetActualBlipArrayIndex(FrontEndMenuManager.m_nTargetBlipIndex);
		if (blipIdx == -1)
		{
			OPCODE_SKIP_PARAMS(3);
			OPCODE_CONDITION_RESULT(false); // no target marker placed
			return OR_CONTINUE;
		}

		CVector pos = CRadar::ms_RadarTrace[blipIdx].m_vecPos; // TODO: support for Fastman92's Limit Adjuster

		// TODO: load world collisions for the coords
		pos.z = CWorld::FindGroundZForCoord(pos.x, pos.z);

		OPCODE_WRITE_PARAM_FLOAT(pos.x);
		OPCODE_WRITE_PARAM_FLOAT(pos.y);
		OPCODE_WRITE_PARAM_FLOAT(pos.z);
		OPCODE_CONDITION_RESULT(true);
		return OR_CONTINUE;
	}

	// get_car_number_of_gears
	// [var numGear: int] = get_car_number_of_gears [Car]
	static OpcodeResult __stdcall opcode_0AB7(CRunningScript* thread)
	{
		auto handle = OPCODE_READ_PARAM_VEHICLE_HANDLE();

		auto vehicle = CPools::GetVehicle(handle);
		auto gears = vehicle->m_pHandlingData->m_transmissionData.m_nNumberOfGears;

		OPCODE_WRITE_PARAM_INT(gears);
		return OR_CONTINUE;
	}

	// get_car_current_gear
	// [var gear : int] = get_car_current_gear [Car]
	static OpcodeResult __stdcall opcode_0AB8(CRunningScript* thread)
	{
		auto handle = OPCODE_READ_PARAM_VEHICLE_HANDLE();

		auto vehicle = CPools::GetVehicle(handle);
		auto gear = vehicle->m_nCurrentGear;

		OPCODE_WRITE_PARAM_INT(gear);
		return OR_CONTINUE;
	}

	// is_car_siren_on
	// is_car_siren_on [Car] (logical)
	static OpcodeResult __stdcall opcode_0ABD(CRunningScript* thread)
	{
		auto handle = OPCODE_READ_PARAM_VEHICLE_HANDLE();

		auto vehicle = CPools::GetVehicle(handle);
		auto state = vehicle->m_nVehicleFlags.bSirenOrAlarm;

		OPCODE_CONDITION_RESULT(state);
		return OR_CONTINUE;
	}

	// is_car_engine_on
	// is_car_engine_on [Car] (logical)
	static OpcodeResult __stdcall opcode_0ABE(CRunningScript* thread)
	{
		auto handle = OPCODE_READ_PARAM_VEHICLE_HANDLE();

		auto vehicle = CPools::GetVehicle(handle);
		auto state = vehicle->m_nVehicleFlags.bEngineOn;

		OPCODE_CONDITION_RESULT(state);
		return OR_CONTINUE;
	}

	// cleo_set_car_engine_on
	// cleo_set_car_engine_on [Car] {state} [bool]
	static OpcodeResult __stdcall opcode_0ABF(CRunningScript* thread)
	{
		auto handle = OPCODE_READ_PARAM_VEHICLE_HANDLE();
		auto state = OPCODE_READ_PARAM_BOOL();

		auto vehicle = CPools::GetVehicle(handle);

		vehicle->m_nVehicleFlags.bEngineOn = (state != false);
		return OR_CONTINUE;
	}
} Instance;
