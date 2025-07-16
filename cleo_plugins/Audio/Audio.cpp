#include "CLEO.h"
#include "CLEO_Utils.h"
#include "plugin.h"
#include "CTheScripts.h"
#include "CSoundSystem.h"
#include "CAudioStream.h"

using namespace CLEO;
using namespace plugin;

#define VALIDATE_STREAM() if(stream != nullptr && !soundSystem.HasStream(stream)) { SHOW_ERROR("Invalid or already closed '0x%X' audio stream handle param in script %s \nScript suspended.", stream, ScriptInfoStr(thread).c_str()); return thread->Suspend(); }

class Audio 
{
public:
    static CSoundSystem soundSystem;

    enum eStreamAction
    {
        Stop,
        Play,
        Pause,
        Resume,
    };

    Audio()
    {
        if (!PluginCheckCleoVersion()) return;

        // register opcodes
        CLEO_RegisterOpcode(0x0AAC, opcode_0AAC); // load_audiostream
        CLEO_RegisterOpcode(0x0AAD, opcode_0AAD); // set_audio_stream_state
        CLEO_RegisterOpcode(0x0AAE, opcode_0AAE); // remove_audio_stream
        CLEO_RegisterOpcode(0x0AAF, opcode_0AAF); // get_audiostream_length
        
        CLEO_RegisterOpcode(0x0AB9, opcode_0AB9); // get_audio_stream_state

        CLEO_RegisterOpcode(0x0ABB, opcode_0ABB); // get_audio_stream_volume
        CLEO_RegisterOpcode(0x0ABC, opcode_0ABC); // set_audio_stream_volume

        CLEO_RegisterOpcode(0x0AC0, opcode_0AC0); // loop_audiostream
        CLEO_RegisterOpcode(0x0AC1, opcode_0AC1); // load_audiostream_with_3d_support
        CLEO_RegisterOpcode(0x0AC2, opcode_0AC2); // set_play_3d_audio_stream_at_coords
        CLEO_RegisterOpcode(0x0AC3, opcode_0AC3); // set_play_3d_audio_stream_at_object
        CLEO_RegisterOpcode(0x0AC4, opcode_0AC4); // set_play_3d_audio_stream_at_char
        CLEO_RegisterOpcode(0x0AC5, opcode_0AC5); // set_play_3d_audio_stream_at_vehicle

        CLEO_RegisterOpcode(0x2500, opcode_2500); // is_audio_stream_playing
        CLEO_RegisterOpcode(0x2501, opcode_2501); // get_audiostream_duration
        CLEO_RegisterOpcode(0x2502, opcode_2502); // get_audio_stream_speed
        CLEO_RegisterOpcode(0x2503, opcode_2503); // set_audio_stream_speed
        CLEO_RegisterOpcode(0x2504, opcode_2504); // set_audio_stream_volume_with_transition
        CLEO_RegisterOpcode(0x2505, opcode_2505); // set_audio_stream_speed_with_transition
        CLEO_RegisterOpcode(0x2506, opcode_2506); // set_audio_stream_source_size
        CLEO_RegisterOpcode(0x2507, opcode_2507); // get_audio_stream_progress
        CLEO_RegisterOpcode(0x2508, opcode_2508); // set_audio_stream_progress

        CLEO_RegisterOpcode(0x2509, opcode_2509); // get_audio_stream_type
        CLEO_RegisterOpcode(0x250A, opcode_250A); // set_audio_stream_type

        CLEO_RegisterOpcode(0x250B, opcode_250B); // get_audio_stream_progress_seconds
        CLEO_RegisterOpcode(0x250C, opcode_250C); // set_audio_stream_progress_seconds

        // register event callbacks
        CLEO_RegisterCallback(eCallbackId::GameBegin, OnGameBegin);
        CLEO_RegisterCallback(eCallbackId::GameProcessAfter, OnGameProcessAfter);
        CLEO_RegisterCallback(eCallbackId::GameEnd, OnGameEnd);
        CLEO_RegisterCallback(eCallbackId::DrawingFinished, OnDrawingFinished);
        CLEO_RegisterCallback(eCallbackId::MainWindowFocus, OnMainWindowFocus);
    }

    ~Audio()
    {
        CLEO_UnregisterCallback(eCallbackId::GameBegin, OnGameBegin);
        CLEO_UnregisterCallback(eCallbackId::GameProcessAfter, OnGameProcessAfter);
        CLEO_UnregisterCallback(eCallbackId::GameEnd, OnGameEnd);
        CLEO_UnregisterCallback(eCallbackId::DrawingFinished, OnDrawingFinished);
        CLEO_UnregisterCallback(eCallbackId::MainWindowFocus, OnMainWindowFocus);
    }

    static void __stdcall OnGameBegin(DWORD saveSlot)
    {
        soundSystem.Init();
    }

    static void __stdcall OnGameProcessAfter()
    {
        soundSystem.Process();
    }

    static void __stdcall OnGameEnd()
    {
        soundSystem.Clear();
    }

    static void __stdcall OnDrawingFinished()
    {
        if (CTimer::m_UserPause) // main menu visible
            soundSystem.Process();
    }

    static void __stdcall OnMainWindowFocus(bool active)
    {
        if (active)
            soundSystem.Resume();
        else
            soundSystem.Pause();
    }

 
    //0AAC=2,  %2d% = load_audiostream %1d%  // IF and SET
    static OpcodeResult __stdcall opcode_0AAC(CLEO::CRunningScript* thread)
    {
        OPCODE_READ_PARAM_STRING_LEN(path, 511);

        if (!isNetworkSource(path))
            CLEO_ResolvePath(thread, _buff_path, sizeof(_buff_path));

        auto ptr = soundSystem.CreateStream(path);

        if (ptr != nullptr && IsLegacyScript(thread))
        {
            ptr->SetType(CLEO::CSoundSystem::LegacyModeDefaultStreamType);
        }

        OPCODE_WRITE_PARAM_PTR(ptr);
        OPCODE_CONDITION_RESULT(ptr != nullptr);
        return OR_CONTINUE;
    }

    //0AAD=2,set_audiostream %1d% perform_action %2d%
    static OpcodeResult __stdcall opcode_0AAD(CLEO::CRunningScript* thread)
    {
        auto stream = (CAudioStream*)OPCODE_READ_PARAM_UINT(); VALIDATE_STREAM()
        auto action = OPCODE_READ_PARAM_INT();

        if (stream)
        {
            switch (action)
            {
                case eStreamAction::Stop: stream->Stop(); break;
                case eStreamAction::Play: stream->Play(); break;
                case eStreamAction::Pause: stream->Pause(); break;
                case eStreamAction::Resume: stream->Resume(); break;
                default:
                    LOG_WARNING(thread, "Unknown AudioStreamAction (%d) in script %s", action, ScriptInfoStr(thread).c_str());
            }
        }

        return OR_CONTINUE;
    }

    //0AAE=1,release_audiostream %1d%
    static OpcodeResult __stdcall opcode_0AAE(CLEO::CRunningScript* thread)
    {
        auto stream = (CAudioStream*)OPCODE_READ_PARAM_UINT(); VALIDATE_STREAM();

        if (stream) soundSystem.DestroyStream(stream);

        return OR_CONTINUE;
    }

    //0AAF=2,%2d% = get_audiostream_length %1d%
    static OpcodeResult __stdcall opcode_0AAF(CLEO::CRunningScript* thread)
    {
        auto stream = (CAudioStream*)OPCODE_READ_PARAM_UINT(); VALIDATE_STREAM();

        auto length = 0.0f;
        if (stream) length = stream->GetLength();

        OPCODE_WRITE_PARAM_INT((int)length);
        return OR_CONTINUE;
    }

    //0AB9=2,get_audio_stream_state %1d% store_to %2d%
    static OpcodeResult __stdcall opcode_0AB9(CLEO::CRunningScript* thread)
    {
        auto stream = (CAudioStream*)OPCODE_READ_PARAM_UINT(); VALIDATE_STREAM();

        auto state = CAudioStream::StreamState::Stopped;
        if (stream) state = stream->GetState();

        OPCODE_WRITE_PARAM_INT(state);
        return OR_CONTINUE;
    }

    //0ABB=2,%2d% = get_audio_stream_volume %1d%
    static OpcodeResult __stdcall opcode_0ABB(CLEO::CRunningScript* thread)
    {
        auto stream = (CAudioStream*)OPCODE_READ_PARAM_UINT(); VALIDATE_STREAM();

        auto volume = 0.0f;
        if (stream) volume = stream->GetVolume();

        OPCODE_WRITE_PARAM_FLOAT(volume);
        return OR_CONTINUE;
    }

    //0ABC=2,set_audiostream %1d% volume %2d%
    static OpcodeResult __stdcall opcode_0ABC(CLEO::CRunningScript* thread)
    {
        auto stream = (CAudioStream*)OPCODE_READ_PARAM_UINT(); VALIDATE_STREAM();
        auto volume = OPCODE_READ_PARAM_FLOAT();

        if (stream) stream->SetVolume(volume);

        return OR_CONTINUE;
    }

    //0AC0=2,loop_audiostream %1d% flag %2d%
    static OpcodeResult __stdcall opcode_0AC0(CLEO::CRunningScript* thread)
    {
        auto stream = (CAudioStream*)OPCODE_READ_PARAM_UINT(); VALIDATE_STREAM();
        auto loop = OPCODE_READ_PARAM_BOOL();

        if (stream) stream->SetLooping(loop);

        return OR_CONTINUE;
    }

    //0AC1=2,%2d% = load_audiostream_with_3d_support %1d% //IF and SET
    static OpcodeResult __stdcall opcode_0AC1(CLEO::CRunningScript* thread)
    {
        OPCODE_READ_PARAM_STRING_LEN(path, 511);

        if (!isNetworkSource(path))
            CLEO_ResolvePath(thread, _buff_path, sizeof(_buff_path));

        auto ptr = soundSystem.CreateStream(path, true);

        if (ptr != nullptr && IsLegacyScript(thread))
        {
            ptr->SetType(CLEO::CSoundSystem::LegacyModeDefaultStreamType);
        }

        OPCODE_WRITE_PARAM_PTR(ptr);
        OPCODE_CONDITION_RESULT(ptr != nullptr);
        return OR_CONTINUE;
    }

    //0AC2=4,set_3d_audiostream %1d% position %2d% %3d% %4d%
    static OpcodeResult __stdcall opcode_0AC2(CLEO::CRunningScript* thread)
    {
        auto stream = (CAudioStream*)OPCODE_READ_PARAM_UINT(); VALIDATE_STREAM();
        CVector pos;
        pos.x = OPCODE_READ_PARAM_FLOAT();
        pos.y = OPCODE_READ_PARAM_FLOAT();
        pos.z = OPCODE_READ_PARAM_FLOAT();

        if (stream) stream->Set3dPosition(pos);

        return OR_CONTINUE;
    }

    //0AC3=2,link_3d_audiostream %1d% to_object %2d%
    static OpcodeResult __stdcall opcode_0AC3(CLEO::CRunningScript* thread)
    {
        auto stream = (CAudioStream*)OPCODE_READ_PARAM_UINT(); VALIDATE_STREAM();
        auto handle = OPCODE_READ_PARAM_OBJECT_HANDLE();

        if (stream)
        {
            auto object = CPools::GetObject(handle);
            stream->SetHost(object, { 0.0f, 0.0f, 0.0f });
        }

        return OR_CONTINUE;
    }

    //0AC4=2,link_3d_audiostream %1d% to_actor %2d%
    static OpcodeResult __stdcall opcode_0AC4(CLEO::CRunningScript* thread)
    {
        auto stream = (CAudioStream*)OPCODE_READ_PARAM_UINT(); VALIDATE_STREAM();
        auto handle = OPCODE_READ_PARAM_PED_HANDLE();

        if (stream)
        {
            auto ped = CPools::GetPed(handle);
            stream->SetHost(ped, { 0.0f, 0.0f, 0.0f });
        }

        return OR_CONTINUE;
    }

    //0AC5=2,link_3d_audiostream %1d% to_vehicle %2d%
    static OpcodeResult __stdcall opcode_0AC5(CLEO::CRunningScript* thread)
    {
        auto stream = (CAudioStream*)OPCODE_READ_PARAM_UINT(); VALIDATE_STREAM();
        auto handle = OPCODE_READ_PARAM_VEHICLE_HANDLE();

        if (stream)
        {
            auto vehicle = CPools::GetVehicle(handle);
            stream->SetHost(vehicle, { 0.0f, 0.0f, 0.0f });
         }

        return OR_CONTINUE;
    }

    //2500=1,  is_audio_stream_playing %1d%
    static OpcodeResult __stdcall opcode_2500(CLEO::CRunningScript* thread)
    {
        auto stream = (CAudioStream*)OPCODE_READ_PARAM_UINT(); VALIDATE_STREAM();

        auto state = CAudioStream::StreamState::Stopped;
        if (stream) state = stream->GetState();

        OPCODE_CONDITION_RESULT(state == CAudioStream::StreamState::Playing);
        return OR_CONTINUE;
    }

    //2501=2,%2d% = get_audiostream_duration %1d%
    static OpcodeResult __stdcall opcode_2501(CLEO::CRunningScript* thread)
    {
        auto stream = (CAudioStream*)OPCODE_READ_PARAM_UINT(); VALIDATE_STREAM();

        auto length = 0.0f;
        if (stream)
        {
            auto length = stream->GetLength();

            auto speed = stream->GetSpeed();
            if (speed <= 0.0f)
                length = FLT_MAX; // it would take forever to play paused
            else
                length /= speed; // speed corrected
        }

        OPCODE_WRITE_PARAM_FLOAT(length);
        return OR_CONTINUE;
    }

    //2502=2,get_audio_stream_speed %1d% store_to %2d%
    static OpcodeResult __stdcall opcode_2502(CLEO::CRunningScript* thread)
    {
        auto stream = (CAudioStream*)OPCODE_READ_PARAM_UINT(); VALIDATE_STREAM();

        auto speed = 0.0f;
        if (stream) speed = stream->GetSpeed();

        OPCODE_WRITE_PARAM_FLOAT(speed);
        return OR_CONTINUE;
    }

    //2503=2,set_audio_stream_speed %1d% speed %2d%
    static OpcodeResult __stdcall opcode_2503(CLEO::CRunningScript* thread)
    {
        auto stream = (CAudioStream*)OPCODE_READ_PARAM_UINT(); VALIDATE_STREAM();
        auto speed = OPCODE_READ_PARAM_FLOAT();

        if (stream) stream->SetSpeed(speed);

        return OR_CONTINUE;
    }

    //2504=3,set_audio_stream_volume_with_transition %1d% volume %2d% time_ms %2d%
    static OpcodeResult __stdcall opcode_2504(CLEO::CRunningScript* thread)
    {
        auto stream = (CAudioStream*)OPCODE_READ_PARAM_UINT(); VALIDATE_STREAM();
        auto volume = OPCODE_READ_PARAM_FLOAT();
        auto time = OPCODE_READ_PARAM_INT();

        if (stream) stream->SetVolume(volume, 0.001f * time);

        return OR_CONTINUE;
    }

    //2505=3,set_audio_stream_speed_with_transition %1d% speed %2d% time_ms %2d%
    static OpcodeResult __stdcall opcode_2505(CLEO::CRunningScript* thread)
    {
        auto stream = (CAudioStream*)OPCODE_READ_PARAM_UINT(); VALIDATE_STREAM();
        auto speed = OPCODE_READ_PARAM_FLOAT();
        auto time = OPCODE_READ_PARAM_INT();

        if (stream) stream->SetSpeed(speed, 0.001f * time);

        return OR_CONTINUE;
    }

    //2506=2,set_audio_stream_source_size %1d% radius %2d%
    static OpcodeResult __stdcall opcode_2506(CLEO::CRunningScript* thread)
    {
        auto stream = (CAudioStream*)OPCODE_READ_PARAM_UINT(); VALIDATE_STREAM();
        auto radius = OPCODE_READ_PARAM_FLOAT();

        if (stream) stream->Set3dSourceSize(radius);

        return OR_CONTINUE;
    }

    //2507=2,get_audio_stream_progress %1d% store_to %2d%
    static OpcodeResult __stdcall opcode_2507(CLEO::CRunningScript* thread)
    {
        auto stream = (CAudioStream*)OPCODE_READ_PARAM_UINT(); VALIDATE_STREAM();

        auto progress = 0.0f;
        if (stream) progress = stream->GetProgress();

        OPCODE_WRITE_PARAM_FLOAT(progress);
        return OR_CONTINUE;
    }

    //2508=2,set_audio_stream_progress %1d% speed %2d%
    static OpcodeResult __stdcall opcode_2508(CLEO::CRunningScript* thread)
    {
        auto stream = (CAudioStream*)OPCODE_READ_PARAM_UINT(); VALIDATE_STREAM();
        auto speed = OPCODE_READ_PARAM_FLOAT();

        if (stream) stream->SetProgress(speed);

        return OR_CONTINUE;
    }

    //2509=2,get_audio_stream_type %1d% store_to %2d%
    static OpcodeResult __stdcall opcode_2509(CLEO::CRunningScript* thread)
    {
        auto stream = (CAudioStream*)OPCODE_READ_PARAM_UINT(); VALIDATE_STREAM();

        auto type = eStreamType::None;
        if (stream) type = stream->GetType();

        OPCODE_WRITE_PARAM_INT(type);
        return OR_CONTINUE;
    }

    //250A=2,set_audio_stream_type %1d% type %2d%
    static OpcodeResult __stdcall opcode_250A(CLEO::CRunningScript* thread)
    {
        auto stream = (CAudioStream*)OPCODE_READ_PARAM_UINT(); VALIDATE_STREAM();
        auto type = OPCODE_READ_PARAM_INT();

        if (stream) stream->SetType((eStreamType)type);

        return OR_CONTINUE;
    }

    //250B=2,get_audio_stream_progress_seconds %1d% store_to %2d%
    static OpcodeResult __stdcall opcode_250B(CLEO::CRunningScript* thread)
    {
        auto stream = (CAudioStream*)OPCODE_READ_PARAM_UINT(); VALIDATE_STREAM();

        auto progress = 0.0f;
        if (stream)
        {
            progress = stream->GetProgress(); // 0.0 - 1.0
            progress *= stream->GetLength();
        }

        OPCODE_WRITE_PARAM_FLOAT(progress);
        return OR_CONTINUE;
    }

    //250C=2,set_audio_stream_progress_seconds %1d% value %2d%
    static OpcodeResult __stdcall opcode_250C(CLEO::CRunningScript* thread)
    {
        auto stream = (CAudioStream*)OPCODE_READ_PARAM_UINT(); VALIDATE_STREAM();
        auto progress = OPCODE_READ_PARAM_FLOAT();

        if (stream)
        {
            float len = stream->GetLength();
            if(len > 0.0f)
            {
                progress /= len;
            }
            else
            {
                progress = 0.0f; // invalid total size
            }

            stream->SetProgress(progress);
        }

        return OR_CONTINUE;
    }
} audioInstance;

CSoundSystem Audio::soundSystem;
