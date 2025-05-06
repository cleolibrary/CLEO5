#include "plugin.h"
#include "CLEO.h"
#include "CLEO_Utils.h"
#include <CCheat.h>
#include <CMenuManager.h>
#include <CModelInfo.h>
#include <CRadar.h>
#include <CWorld.h>
#include <map>

using namespace CLEO;
using namespace plugin;
using namespace std;

class GameEntities
{
public:
	std::map<CRunningScript*, int> charSearchState; // for get_random_char_in_sphere_no_save_recursive
	std::map<CRunningScript*, int> carSearchState; // for get_random_car_in_sphere_no_save_recursive
	std::map<CRunningScript*, int> objectSearchState; // for get_random_object_in_sphere_no_save_recursive

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
		CLEO_RegisterOpcode(0x0AD2, opcode_0AD2); // get_char_player_is_targeting
		CLEO_RegisterOpcode(0x0ADD, opcode_0ADD); // spawn_vehicle_by_cheating
		CLEO_RegisterOpcode(0x0AE1, opcode_0AE1); // get_random_char_in_sphere_no_save_recursive
		CLEO_RegisterOpcode(0x0AE2, opcode_0AE2); // get_random_car_in_sphere_no_save_recursive
		CLEO_RegisterOpcode(0x0AE3, opcode_0AE3); // get_random_object_in_sphere_no_save_recursive

		// register event callbacks
		CLEO_RegisterCallback(eCallbackId::ScriptsFinalize, OnScriptsFinalize);
	}

	~GameEntities()
	{
		CLEO_UnregisterCallback(eCallbackId::ScriptsFinalize, OnScriptsFinalize);
	}

	static void __stdcall OnScriptsFinalize()
	{
		Instance.charSearchState.clear();
		Instance.carSearchState.clear();
		Instance.objectSearchState.clear();
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
			OPCODE_SKIP_PARAMS(3); // x, y, z
			OPCODE_CONDITION_RESULT(false); // no target marker placed
			return OR_CONTINUE;
		}

		CVector pos = CRadar::ms_RadarTrace[blipIdx].m_vecPos; // TODO: support for Fastman92's Limit Adjuster

		// TODO: load world collisions for the coords
		pos.z = CWorld::FindGroundZForCoord(pos.x, pos.y);

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

	// get_char_player_is_targeting
	// [var handle: Char] = get_char_player_is_targeting [Player] (logical)
	static OpcodeResult __stdcall opcode_0AD2(CRunningScript* thread)
	{
		auto playerId = OPCODE_READ_PARAM_PLAYER_ID();

		auto ped = FindPlayerPed(playerId);

		auto target = ped->m_pPlayerTargettedPed;
		if (target == nullptr) target = (CPed*)ped->m_pTargetedObject;
		
		if (target == nullptr || target->m_nType != ENTITY_TYPE_PED)
		{
			OPCODE_WRITE_PARAM_INT(-1);
			OPCODE_CONDITION_RESULT(false);
			return OR_CONTINUE;
		}

		auto handle = CPools::GetPedRef(target);

		OPCODE_WRITE_PARAM_INT(handle);
		OPCODE_CONDITION_RESULT(true);
		return OR_CONTINUE;
	}

	// spawn_vehicle_by_cheating
	// spawn_vehicle_by_cheating {modelId} [model_vehicle]
	static OpcodeResult __stdcall opcode_0ADD(CRunningScript* thread)
	{
		auto modelIndex = OPCODE_READ_PARAM_INT();

		auto model = CModelInfo::GetModelInfo(modelIndex);
		if (model == nullptr || model->GetModelType() != MODEL_INFO_VEHICLE)
		{
			return OR_CONTINUE; // modelIndex is not vehicle
		}

		auto veh = (CVehicleModelInfo*)model;
		switch (veh->m_nVehicleType)
		{
			case VEHICLE_AUTOMOBILE:
			case VEHICLE_MTRUCK:
			case VEHICLE_QUAD:
			case VEHICLE_HELI:
			case VEHICLE_PLANE:
			case VEHICLE_BOAT:
			//case VEHICLE_TRAIN:
			//case VEHICLE_FHELI:
			//case VEHICLE_FPLANE:
			case VEHICLE_BIKE:
			case VEHICLE_BMX:
			case VEHICLE_TRAILER:
				break;

			default:
				return OR_CONTINUE; // unsupported vehicle type
		}

		CCheat::VehicleCheat(modelIndex);

		return OR_CONTINUE;
	}

	// get_random_char_in_sphere_no_save_recursive
	// [var handle: Char] = get_random_char_in_sphere_no_save_recursive {x} [float] {y} [float] {z} [float] {radius} [float] {findNext} [bool] {filter} [int] (logical)
	static OpcodeResult __stdcall opcode_0AE1(CRunningScript* thread)
	{
		CVector center = {};
		center.x = OPCODE_READ_PARAM_FLOAT();
		center.y = OPCODE_READ_PARAM_FLOAT();
		center.z = OPCODE_READ_PARAM_FLOAT();
		auto radius = OPCODE_READ_PARAM_FLOAT();
		auto findNext = OPCODE_READ_PARAM_BOOL();
		auto filter = OPCODE_READ_PARAM_INT();

		bool skipDead = (filter == 1);
		bool skipPlayer = (filter != -1);

		int& searchIdx = Instance.charSearchState[thread];
		if (!findNext) searchIdx = 0;

		CPed* found = nullptr;
		for (int index = searchIdx; index < CPools::ms_pPedPool->m_nSize; index++)
		{
			auto ped = CPools::ms_pPedPool->GetAt(index);

			if (ped == nullptr || ped->m_nPedFlags.bFadeOut)
			{
				continue; // invalid or about to be deleted
			}

			if (skipPlayer && ped->IsPlayer())
			{
				continue; // player
			}

			if (skipDead)
			{
				if (ped->m_ePedState == PEDSTATE_DIE || ped->m_ePedState == PEDSTATE_DEAD)
				{
					continue; // dead
				}
			}

			if (radius < 1000.0f)
			{
				if((ped->GetPosition() - center).Magnitude() > radius)
				{
					continue; // out of search radius
				}
			}

			found = ped;
			searchIdx = index + 1; // next search start index
			break;
		}

		DWORD handle;
		if (found != nullptr)
		{
			handle = CPools::ms_pPedPool->GetRef(found);
		}
		else
		{
			handle = -1;
			searchIdx = 0;
		}

		OPCODE_WRITE_PARAM_INT(handle);
		OPCODE_CONDITION_RESULT(handle != -1);
		return OR_CONTINUE;
	}

	// get_random_car_in_sphere_no_save_recursive
	// [var handle: Car] = get_random_car_in_sphere_no_save_recursive {x} [float] {y} [float] {z} [float] {radius} [float] {findNext} [bool] {skipWrecked} [bool] (logical)
	static OpcodeResult __stdcall opcode_0AE2(CRunningScript* thread)
	{
		CVector center = {};
		center.x = OPCODE_READ_PARAM_FLOAT();
		center.y = OPCODE_READ_PARAM_FLOAT();
		center.z = OPCODE_READ_PARAM_FLOAT();
		auto radius = OPCODE_READ_PARAM_FLOAT();
		auto findNext = OPCODE_READ_PARAM_BOOL();
		auto skipWrecked = OPCODE_READ_PARAM_BOOL();

		int& searchIdx = Instance.carSearchState[thread];
		if (!findNext) searchIdx = 0;

		CVehicle* found = nullptr;
		for (int index = searchIdx; index < CPools::ms_pVehiclePool->m_nSize; index++)
		{
			auto car = CPools::ms_pVehiclePool->GetAt(index);

			if (car == nullptr || car->m_nVehicleFlags.bFadeOut)
			{
				continue; // invalid or about to be deleted
			}

			if (skipWrecked)
			{
				if (car->m_nStatus == STATUS_WRECKED || car->m_nVehicleFlags.bIsDrowning)
				{
					continue; // wrecked
				}
			}

			if (radius < 1000.0f)
			{
				if ((car->GetPosition() - center).Magnitude() > radius)
				{
					continue; // out of search radius
				}
			}

			found = car;
			searchIdx = index + 1; // next search start index
			break;
		}

		DWORD handle;
		if (found != nullptr)
		{
			handle = CPools::ms_pVehiclePool->GetRef(found);
		}
		else
		{
			handle = -1;
			searchIdx = 0;
		}

		OPCODE_WRITE_PARAM_INT(handle);
		OPCODE_CONDITION_RESULT(handle != -1);
		return OR_CONTINUE;
	}

	// get_random_object_in_sphere_no_save_recursive
	// [var handle: Object] = get_random_object_in_sphere_no_save_recursive {x} [float] {y} [float] {z} [float] {radius} [float] {findNext} [bool] (logical)
	static OpcodeResult __stdcall opcode_0AE3(CRunningScript* thread)
	{
		CVector center = {};
		center.x = OPCODE_READ_PARAM_FLOAT();
		center.y = OPCODE_READ_PARAM_FLOAT();
		center.z = OPCODE_READ_PARAM_FLOAT();
		auto radius = OPCODE_READ_PARAM_FLOAT();
		auto findNext = OPCODE_READ_PARAM_BOOL();

		int& searchIdx = Instance.objectSearchState[thread];
		if (!findNext) searchIdx = 0;

		CObject* found = nullptr;
		for (int index = searchIdx; index < CPools::ms_pObjectPool->m_nSize; index++)
		{
			auto object = CPools::ms_pObjectPool->GetAt(index);

			if (object == nullptr || object->m_nObjectFlags.bFadingIn) // this is actually .bFadingOut
			{
				continue; // invalid
			}

			if (radius < 1000.0f)
			{
				if ((object->GetPosition() - center).Magnitude() > radius)
				{
					continue; // out of search radius
				}
			}

			found = object;
			searchIdx = index + 1; // next search start index
			break;
		}

		DWORD handle;
		if (found != nullptr)
		{
			handle = CPools::ms_pObjectPool->GetRef(found);
		}
		else
		{
			handle = -1;
			searchIdx = 0;
		}

		OPCODE_WRITE_PARAM_INT(handle);
		OPCODE_CONDITION_RESULT(handle != -1);
		return OR_CONTINUE;
	}
} Instance;
