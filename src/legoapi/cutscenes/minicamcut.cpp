#include <string.h>
#include "legoapi/characters/motion.h"
#include "legoapi/render/fx/spline_position.h"
#include "nu2api/numath/nutrig.h"

#include "globals.h"
#include "legoapi/cutscenes/minicamcut.h"
#include "legoapi/legoapi_types.h"
#include "nu2api/numath/nuvec.h"

void FindAnglesXY(nuvec_s *, u16 *, u16 *);
MINICAM_s MiniCam = {};

void Minicam_Update() {
    f32 distance = MiniCam.distance;
    i32 pitch = MiniCam.pitch;
    i32 yaw = MiniCam.yaw;
    i32 roll = MiniCam.roll;
    NUVEC *focus = MiniCam.focus;
    NUVEC position = MiniCam.position;
    i32 pitch_rate = 0, yaw_rate = 0, roll_rate = 0;
    f32 command_duration;
read_command:
    if (MiniCam.current_command < MiniCam.command_count)
        goto dispatch_command;
    command_duration = 0.0f;
execute_command:
    MiniCam.elapsed_time += FRAMETIME;
    if (MiniCam.duration != 0.0f && MiniCam.elapsed_time > MiniCam.duration) {
        Minicam_AddDeltas(command_duration);
        goto end_cut;
    }
    if (MiniCam.current_command >= MiniCam.command_count) {
        Minicam_AddDeltas(1000000000.0f);
        Minicam_CalcCamPos();
        return;
    }
    if (MiniCam.commands[MiniCam.current_command].type == 1) {
        if (command_duration > MiniCam.delta_time) {
            MiniCam.delta_time += FRAMETIME;
        } else {
            NUVEC delta;
            MiniCam.distance = MiniCam.start_distance =
                NuVecDist(NUMTX_GET_ROW_VEC(&GameCam->render_mtx, 3), MiniCam.focus, &delta);
            MiniCam.pitch = MiniCam.start_pitch = GameCam->pitch + 0x8000;
            MiniCam.yaw = MiniCam.start_yaw = GameCam->yaw;
            MiniCam.position = GameCam->pos;
            Minicam_ResetForNextCommand();
        }
        return;
    }
    if (MiniCam.commands[MiniCam.current_command].type == 3) {
        if (command_duration > MiniCam.delta_time) {
            MiniCam.delta_time += FRAMETIME;
            Minicam_AddDeltas(command_duration);
            Minicam_CalcCamPos();
            return;
        }
        ObstacleCamAlwaysSnapAngles = 0;
        goto end_cut;
    }
    if (MiniCam.commands[MiniCam.current_command].type == 4 && MiniCam.delta_time == 0.0f) {
        if (command_duration == 0.0f) {
            MiniCam.focus = focus;
            MiniCam.distance = distance;
            MiniCam.pitch = pitch;
            MiniCam.yaw = yaw;
            MiniCam.roll = roll;
            MiniCam.focus_offset = v000;
            MiniCam.position = position;
            Minicam_ClearDeltas();
            GameCam->mode = -1;
        } else {
            MiniCam.target_pitch = pitch_rate != 0 ? (i32)(pitch_rate * command_duration) : pitch;
            MiniCam.start_pitch = MiniCam.pitch;
            MiniCam.target_yaw = yaw_rate != 0 ? (i32)(yaw_rate * command_duration) : yaw;
            MiniCam.start_yaw = MiniCam.yaw;
            MiniCam.target_roll = roll_rate != 0 ? (i32)(roll_rate * command_duration) : roll;
            MiniCam.target_distance = distance;
            MiniCam.start_roll = MiniCam.roll;
            MiniCam.start_distance = MiniCam.distance;
            if (MiniCam.focus != focus) {
                NuVecSub(&MiniCam.focus_offset, MiniCam.focus, focus);
                NuVecScale(&MiniCam.focus_velocity, &MiniCam.focus_offset, 1.0f / command_duration);
                MiniCam.focus = focus;
            }
            NuVecSub(&MiniCam.position_velocity, &position, &MiniCam.position);
            NuVecScale(&MiniCam.position_velocity, &MiniCam.position_velocity, 1.0f / command_duration);
        }
    }
    MiniCam.delta_time += FRAMETIME;
    if (MiniCam.delta_time >= command_duration) {
        Minicam_AddDeltas(command_duration);
        Minicam_ResetForNextCommand();
    } else {
        Minicam_AddDeltas(command_duration);
    }
    Minicam_CalcCamPos();
    return;
end_cut:
    Minicam_CalcCamPos();
    ObstacleCamEnd = 0.0f;
    if (ObstacleCamBlendOutTime == 0.0f) {
        GameCam->mode = -1;
        Minicam_ResetForNextCommand();
    }
    if (MiniCutCam == 0)
        Minicam_ResetForNextCommand();
    return;
dispatch_command: {
    MINICAMCOMMAND_s *command = &MiniCam.commands[MiniCam.current_command];
    switch (command->type) {
        case 0:
            return;
        case 1:
        case 3:
        case 4:
            command_duration = command->duration;
            goto execute_command;
        case 2:
            command_duration = 0.0f;
            goto execute_command;
        case 5:
            command_duration = command->duration;
            Minicam_ClearDeltas();
            goto execute_command;
        case 6: {
            MiniCam_ChangeMode(command->argument);
            u16 target_pitch = pitch_rate != 0 ? (i32)(pitch_rate * 0.0f) : pitch;
            u16 old_yaw = MiniCam.target_yaw;
            u16 old_roll = MiniCam.target_roll;
            f32 old_distance = MiniCam.target_distance;
            MiniCam.target_pitch = target_pitch;
            MiniCam.start_pitch = MiniCam.pitch;
            Minicam_ClearDeltas();
            if (pitch == target_pitch)
                pitch = MiniCam.target_pitch;
            if (yaw == old_yaw)
                yaw = MiniCam.target_yaw;
            if (roll == old_roll)
                roll = MiniCam.target_roll;
            if (distance == old_distance)
                distance = MiniCam.target_distance;
            break;
        }
        case 7:
            MiniCam.easing = command->argument;
            break;
        case 8:
            focus = command->target;
            break;
        case 9:
            distance = command->duration;
            break;
        case 10:
            pitch = command->argument;
            break;
        case 11:
            yaw = command->argument;
            break;
        case 12:
            roll = command->argument;
            break;
        case 13:
            pitch_rate = command->argument;
            break;
        case 14:
            yaw_rate = command->argument;
            break;
        case 15:
            roll_rate = command->argument;
            break;
        case 16:
            position = command->position;
            break;
        case 17:
            MiniCam.position_source = command->target;
            break;
        default:
            ++MiniCam.current_command;
            return;
    }
    ++MiniCam.current_command;
    goto read_command;
}
}

void Minicam_AddDeltas(float duration) {
    f32 fraction;
    if ((duration == 0.0f) | (MiniCam.delta_time == 0.0f)) {
        fraction = 0.0f;
    } else {
        fraction = MiniCam.delta_time / duration;
        if (fraction < 0.0f)
            fraction = 0.0f;
        else if (fraction > 1.0f)
            fraction = 1.0f;
    }
    switch (MiniCam.easing) {
        case 1:
            fraction = 1.0f + NU_SIN_LUT(fraction * 16384.0f + 32768.0f + 16384.0f);
            break;
        case 2:
            fraction = 1.0f - (NU_SIN_LUT(fraction * 32768.0f + 16384.0f) + 1.0f) * 0.5f;
            break;
        case 3:
            fraction = NU_SIN_LUT(fraction * 16384.0f + 49152.0f + 16384.0f);
            break;
        case 4:
            if (fraction < 0.5f) {
                fraction = NU_SIN_LUT((fraction + fraction) * 16384.0f) * 0.5f;
            } else if (fraction > 0.5f) {
                fraction =
                    (1.0f - NU_SIN_LUT(((fraction - 0.5f) + (fraction - 0.5f)) * 16384.0f + 16384.0f)) * 0.5f + 0.5f;
            } else {
                fraction = 0.5f;
            }
            break;
    }
    if (MiniCam.mode == 0) {
        i32 pitch_delta = MiniCam.target_pitch - MiniCam.start_pitch;
        if (pitch_delta > 32768)
            pitch_delta -= 65536;
        i32 yaw_delta = MiniCam.target_yaw - MiniCam.start_yaw;
        if (yaw_delta > 32768)
            yaw_delta -= 65536;
        i32 roll_delta = MiniCam.target_roll - MiniCam.start_roll;
        if (roll_delta > 32768)
            roll_delta -= 65536;
        MiniCam.pitch = SeekRot(MiniCam.pitch, (u16)(i32)(MiniCam.start_pitch + pitch_delta * fraction), 10.0f);
        MiniCam.yaw = SeekRot(MiniCam.yaw, (u16)(i32)(MiniCam.start_yaw + yaw_delta * fraction), 10.0f);
        MiniCam.roll = SeekRot(MiniCam.roll, (u16)(i32)(MiniCam.start_roll + roll_delta * fraction), 10.0f);
        MiniCam.distance = (MiniCam.target_distance - MiniCam.start_distance) * fraction + MiniCam.start_distance;
    } else if (MiniCam.mode == 1) {
        MiniCam.position.x += MiniCam.position_velocity.x * FRAMETIME;
        MiniCam.position.y += MiniCam.position_velocity.y * FRAMETIME;
        MiniCam.position.z += MiniCam.position_velocity.z * FRAMETIME;
    } else if (MiniCam.mode == 2) {
        PointAlongSpline(MiniCam.position_spline, fraction, &MiniCam.position, NULL, NULL, 0);
    }
    MiniCam.focus_offset.x -= MiniCam.focus_velocity.x * FRAMETIME;
    MiniCam.focus_offset.y -= MiniCam.focus_velocity.y * FRAMETIME;
    MiniCam.focus_offset.z -= MiniCam.focus_velocity.z * FRAMETIME;
    if (MiniCam.focus != NULL) {
        NUVEC target;
        NuVecAdd(&target, MiniCam.focus, &MiniCam.focus_offset);
        MiniCam.target.x = SeekLinearF(MiniCam.target.x, target.x, 10.0f);
        MiniCam.target.y = SeekLinearF(MiniCam.target.y, target.y, 10.0f);
        MiniCam.target.z = SeekLinearF(MiniCam.target.z, target.z, 10.0f);
    }
}

void Minicam_AddCommand(i32 type, f32 duration, i32 argument, void *target, nuvec_s position) {
    if (type == 6 || type == 7) {
        i32 i = MiniCam.command_count;
        for (; i >= 0 && MiniCam.commands[i].type != type; --i) {
        }
        if (i != -1 && MiniCam.commands[i].argument == argument)
            return;
    }
    if (MiniCam.command_count == 32)
        return;
    switch (type) {
        case 1:
            MiniCam.commands[MiniCam.command_count].type = 1;
            MiniCam.commands[MiniCam.command_count].argument = 0;
            MiniCam.commands[MiniCam.command_count].duration = duration;
            MiniCam.commands[MiniCam.command_count].position = v000;
            MiniCam.commands[MiniCam.command_count].target = NULL;
            break;
        case 3:
            MiniCam.commands[MiniCam.command_count].type = 3;
            MiniCam.commands[MiniCam.command_count].argument = 0;
            MiniCam.commands[MiniCam.command_count].duration = duration;
            MiniCam.commands[MiniCam.command_count].position = v000;
            MiniCam.commands[MiniCam.command_count].target = NULL;
            break;
        case 4:
            MiniCam.commands[MiniCam.command_count].type = 4;
            MiniCam.commands[MiniCam.command_count].argument = 0;
            MiniCam.commands[MiniCam.command_count].duration = duration;
            MiniCam.commands[MiniCam.command_count].position = v000;
            MiniCam.commands[MiniCam.command_count].target = NULL;
            break;
        case 5:
            MiniCam.commands[MiniCam.command_count].type = 5;
            MiniCam.commands[MiniCam.command_count].argument = 0;
            MiniCam.commands[MiniCam.command_count].duration = duration;
            MiniCam.commands[MiniCam.command_count].position = v000;
            MiniCam.commands[MiniCam.command_count].target = NULL;
            break;
        case 6:
            MiniCam.commands[MiniCam.command_count].type = 6;
            MiniCam.commands[MiniCam.command_count].argument = argument;
            MiniCam.commands[MiniCam.command_count].duration = 0.0f;
            MiniCam.commands[MiniCam.command_count].position = v000;
            MiniCam.commands[MiniCam.command_count].target = NULL;
            break;
        case 7:
            MiniCam.commands[MiniCam.command_count].type = 7;
            MiniCam.commands[MiniCam.command_count].argument = argument;
            MiniCam.commands[MiniCam.command_count].duration = 0.0f;
            MiniCam.commands[MiniCam.command_count].position = v000;
            MiniCam.commands[MiniCam.command_count].target = NULL;
            break;
        case 8:
            MiniCam.commands[MiniCam.command_count].type = 8;
            MiniCam.commands[MiniCam.command_count].argument = 0;
            MiniCam.commands[MiniCam.command_count].duration = 0.0f;
            MiniCam.commands[MiniCam.command_count].position = v000;
            MiniCam.commands[MiniCam.command_count].target = (NUVEC *)target;
            break;
        case 9:
            MiniCam.commands[MiniCam.command_count].type = 9;
            MiniCam.commands[MiniCam.command_count].argument = 0;
            MiniCam.commands[MiniCam.command_count].duration = duration;
            MiniCam.commands[MiniCam.command_count].position = v000;
            MiniCam.commands[MiniCam.command_count].target = NULL;
            break;
        case 10:
            MiniCam.commands[MiniCam.command_count].type = 10;
            MiniCam.commands[MiniCam.command_count].argument = argument;
            MiniCam.commands[MiniCam.command_count].duration = 0.0f;
            MiniCam.commands[MiniCam.command_count].position = v000;
            MiniCam.commands[MiniCam.command_count].target = NULL;
            break;
        case 11:
            MiniCam.commands[MiniCam.command_count].type = 11;
            MiniCam.commands[MiniCam.command_count].argument = argument;
            MiniCam.commands[MiniCam.command_count].duration = 0.0f;
            MiniCam.commands[MiniCam.command_count].position = v000;
            MiniCam.commands[MiniCam.command_count].target = NULL;
            break;
        case 12:
            MiniCam.commands[MiniCam.command_count].type = 12;
            MiniCam.commands[MiniCam.command_count].argument = argument;
            MiniCam.commands[MiniCam.command_count].duration = 0.0f;
            MiniCam.commands[MiniCam.command_count].position = v000;
            MiniCam.commands[MiniCam.command_count].target = NULL;
            break;
        case 13:
            MiniCam.commands[MiniCam.command_count].type = 13;
            MiniCam.commands[MiniCam.command_count].argument = argument;
            MiniCam.commands[MiniCam.command_count].duration = 0.0f;
            MiniCam.commands[MiniCam.command_count].position = v000;
            MiniCam.commands[MiniCam.command_count].target = NULL;
            break;
        case 14:
            MiniCam.commands[MiniCam.command_count].type = 14;
            MiniCam.commands[MiniCam.command_count].argument = argument;
            MiniCam.commands[MiniCam.command_count].duration = 0.0f;
            MiniCam.commands[MiniCam.command_count].position = v000;
            MiniCam.commands[MiniCam.command_count].target = NULL;
            break;
        case 15:
            MiniCam.commands[MiniCam.command_count].type = 15;
            MiniCam.commands[MiniCam.command_count].argument = argument;
            MiniCam.commands[MiniCam.command_count].duration = 0.0f;
            MiniCam.commands[MiniCam.command_count].position = v000;
            MiniCam.commands[MiniCam.command_count].target = NULL;
            break;
        case 16:
            MiniCam.commands[MiniCam.command_count].type = 16;
            MiniCam.commands[MiniCam.command_count].argument = 0;
            MiniCam.commands[MiniCam.command_count].duration = 0.0f;
            MiniCam.commands[MiniCam.command_count].position = position;
            MiniCam.commands[MiniCam.command_count].target = NULL;
            break;
        case 17:
            MiniCam.commands[MiniCam.command_count].type = 17;
            MiniCam.commands[MiniCam.command_count].argument = 0;
            MiniCam.commands[MiniCam.command_count].duration = 0.0f;
            MiniCam.commands[MiniCam.command_count].position = v000;
            MiniCam.commands[MiniCam.command_count].target = (NUVEC *)target;
            break;
    }
    ++MiniCam.command_count;
}

void Minicam_CalcCamPos() {
    if (MiniCam.mode == 0) {
        MiniCam.position.x = MiniCam.position.y = 0.0f;
        MiniCam.position.z = MiniCam.distance;
        NuVecRotateX(&MiniCam.position, &MiniCam.position, MiniCam.pitch);
        NuVecRotateY(&MiniCam.position, &MiniCam.position, MiniCam.yaw);
        ObstacleCamRotZ = MiniCam.roll;
        NuVecAdd(&MiniCam.position, &MiniCam.position, &MiniCam.target);
    }
}

void Minicam_InitSystem() {
    memset(&MiniCam, 0, sizeof(MiniCam));
}

void Minicam_ClearDeltas() {
    MiniCam.target_distance = MiniCam.start_distance = MiniCam.distance;
    MiniCam.target_pitch = MiniCam.start_pitch = MiniCam.pitch;
    MiniCam.target_yaw = MiniCam.start_yaw = MiniCam.yaw;
    MiniCam.target_roll = MiniCam.start_roll = MiniCam.roll;
    MiniCam.focus_velocity = MiniCam.focus_offset = v000;
}

void Minicam_ResetForNewCut() {
    memset(&MiniCam, 0, sizeof(MiniCam));
}

void Minicam_ResetForNextCommand() {
    Minicam_ClearDeltas();

    const i32 current_command = MiniCam.current_command;
    const i32 command_count = MiniCam.command_count;
    const i32 next_command = current_command + 1;
    i32 remaining_commands;
    if (next_command < command_count) {
        MINICAMCOMMAND_s *source = &MiniCam.commands[next_command];
        MINICAMCOMMAND_s *destination = MiniCam.commands;
        MINICAMCOMMAND_s *const command_end = &MiniCam.commands[command_count];
        while (source != command_end) {
            *destination++ = *source++;
        }
        remaining_commands = command_count - next_command;
    } else {
        remaining_commands = 0;
    }

    MiniCam.command_count = static_cast<u8>(remaining_commands);
    memset(&MiniCam.commands[remaining_commands], 0,
           static_cast<usize>(32 - remaining_commands) * sizeof(MINICAMCOMMAND_s));
    MiniCam.current_command = 0;
    MiniCam.delta_time = 0.0f;
}

void MiniCam_ChangeMode(i32 mode) {
    if (mode == 0) {
        NUVEC delta;
        MiniCam.distance = NuVecDist(&MiniCam.target, &MiniCam.position, &delta);
        FindAnglesXY(&delta, &MiniCam.pitch, &MiniCam.yaw);
        MiniCam.yaw += 0x8000;
        MiniCam.pitch = -MiniCam.pitch;
        MiniCam.mode = 0;
    } else if (mode == 1) {
        MiniCam.mode = 1;
    } else if (mode == 2) {
        MiniCam.mode = 2;
    }
}
