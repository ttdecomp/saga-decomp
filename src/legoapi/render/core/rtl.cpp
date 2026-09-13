#include "decomp.h"
#include "legoapi/render/core/rtl.h"
#include "globals.h"
#include "legoapi/legoapi_types.h"
#include "nu2api/nu3d/nucamera.h"
#include "nu2api/nu3d/android/nutimebar_plain.h"
#include "nu2api/nu3d/nurndrstat.h"
#include "nu2api/nufile/nufile.h"
#include "nu2api/nucore/nulst.h"

#include <math.h>
#include <string.h>

struct nuqtdim_s;
struct nuqthdr_s;
struct rtl_s;
struct rtlidata_s {
    union {
        u8 data[sizeof(rtldata_s)];
        struct {
            u8 reserved_00[0x4c];
            rtl_s *cached_light;
            u8 reserved_50[0x0c];
            f32 cached_value;
            u8 reserved_60[0x14];
            u16 cached_light_uid;
            u8 reserved_76[0xce];
        };
    };
};
DECOMP_ASSERT(sizeof(rtlidata_s) == 0x144, "rtlidata_s size");
DECOMP_ASSERT(offsetof(rtlidata_s, cached_light) == 0x4c, "rtlidata_s cached light offset");
DECOMP_ASSERT(offsetof(rtlidata_s, cached_light_uid) == 0x74, "rtlidata_s cached UID offset");
struct NUFRUSTRUM;

static NULSTHDR *rtl_dynamic_pool;
static i32 rtl_dynamic_lights_enabled = 1;
static i32 rtl_dynamic_max;
static i32 rtl_dynamic_cnt;
static i16 rtl_uid = 1;

extern "C" {
    rtlset *curr_set = NULL;
}

extern "C" {
    static void NuVecClear(NUVEC *v) {
        v->x = v->y = v->z = 0.0f;
    }
}

void rtlSwapSetEndianess(rtlset *);

static __used__ rtl_s *GetNextRTL(void *, rtl_s *, char *, int *) {
    return nullptr;
}

static __used__ int InsertData(nuqthdr_s *, int, void *) {
    return 0;
}

static __used__ void InsertLight(rtl_s *, rtlidata_s *, float) {
}

static __used__ void InsertAntiLight(rtl_s *, rtlidata_s *, float) {
}

static __used__ int FindNearestRTL(nuvec_s *, int) {
    return 0;
}

static __used__ bool InsideLineXZ(float, float, float, float, float, float) {
    return false;
}

static f32 ClampUnit(f32 value) {
    if (value < 0.0f) {
        return 0.0f;
    }
    return value > 1.0f ? 1.0f : value;
}

static __used__ int FindNearestFog(nuvec_s *) {
    return 0;
}

static __used__ i32 rtlCalcLights(nuvec_s *, numtx_s *, f32, rtlidata_s *) {
    return 0;
}

static __used__ void rtlCalcShadow(rtlidata_s *) {
}

static __used__ void rtlProcessLight(rtl_s *, f32) {
}

static __used__ void rtlSwapEndianess32(void *) {
}

static void rtlApplySetScaleLoop(void *, rtlidata_s *, nuvec_s *, numtx_s *, i32, f32);

static __used__ void rtlApplyModifiersToChainLight(rtl_s *) {
}

static __used__ void rtlApplyModifiersToSingleLight(rtl_s *) {
}

static __used__ i32 rtlCmp(rtl_s *, rtl_s *) {
    return 0;
}

extern "C" {

    void IndexLights(rtlset *, VARIPTR *, i32);
    i32 NuRndrSetAmbientLightPS(const NUCOLOUR3 *);
    i32 NuRndrSetDirectionalLightsPS(const NUVEC *, const NUCOLOUR3 *, const NUVEC *, const NUCOLOUR3 *, const NUVEC *,
                                     const NUCOLOUR3 *);

    void fogAlloc(void) {
    }

    void fogFree(void) {
    }

    void rtlAlloc(void) {
    }

    void rtlResetEx(rtldata_s *data, i32 reset_cached) {
        memset(data, 0, 0x48);
        data->data[0x120] = 0;
        *reinterpret_cast<f32 *>(data->data + 0x120) = 1.0f;
        memset(data->data + 0x78, 0, 0x24);
        const NUVEC default_direction = {1.0f, 0.0f, 0.0f};
        for (i32 i = 0; i < 3; ++i) {
            *reinterpret_cast<NUVEC *>(data->data + 0x9c + i * sizeof(NUVEC)) = default_direction;
        }
        *reinterpret_cast<f32 *>(data->data + 0x134) = 1.0f;
        *reinterpret_cast<f32 *>(data->data + 0x138) = 0.0f;
        *reinterpret_cast<f32 *>(data->data + 0x13c) = 0.0f;
        if (reset_cached != 0) {
            memset(data->data + 0x4c, 0, 0x2c);
            *reinterpret_cast<f32 *>(data->data + 0x130) = 0.0f;
        }
    }

    static void rtlInsertLight(u8 *light, rtldata_s *data, f32 strength) {
        const bool ambient = light[0x58] == 1;
        const i32 pointer_offset = ambient ? 0x18 : 0x00;
        const i32 strength_offset = ambient ? 0x24 : 0x0c;
        for (i32 slot = 0; slot < 3; ++slot) {
            if (*reinterpret_cast<f32 *>(data->data + strength_offset + slot * 4) < strength) {
                for (i32 move = 2; move > slot; --move) {
                    *reinterpret_cast<u8 **>(data->data + pointer_offset + move * 4) =
                        *reinterpret_cast<u8 **>(data->data + pointer_offset + (move - 1) * 4);
                    *reinterpret_cast<f32 *>(data->data + strength_offset + move * 4) =
                        *reinterpret_cast<f32 *>(data->data + strength_offset + (move - 1) * 4);
                }
                *reinterpret_cast<u8 **>(data->data + pointer_offset + slot * 4) = light;
                *reinterpret_cast<f32 *>(data->data + strength_offset + slot * 4) = strength;
                return;
            }
        }
    }

    static f32 rtlDistanceStrength(const u8 *light, const NUVEC *position) {
        const NUVEC *light_position = reinterpret_cast<const NUVEC *>(light);
        const f32 dx = position->x - light_position->x;
        const f32 dy = position->y - light_position->y;
        const f32 dz = position->z - light_position->z;
        const f32 inner = *reinterpret_cast<const f32 *>(light + 0x34);
        const f32 outer = *reinterpret_cast<const f32 *>(light + 0x40);
        const f32 distance_sq = dx * dx + dy * dy + dz * dz;
        if (distance_sq >= outer * outer) {
            return 0.0f;
        }
        if (inner >= outer) {
            return 1.0f;
        }
        const f32 distance = sqrtf(distance_sq);
        return ClampUnit(1.0f - (distance - inner) / (outer - inner));
    }

} // extern "C"

static void rtlApplySetScaleLoop(void *set, rtlidata_s *lighting_data, NUVEC *position, NUMTX *rotation, i32 identity,
                                 f32 scale) {
    (void)identity;
    rtldata_s *data = reinterpret_cast<rtldata_s *>(lighting_data);
    if (set != NULL) {
        u8 *light = static_cast<u8 *>(set) + 4;
        for (i32 i = 0; i < 0x80 && light[0x58] != 0; ++i, light += 0x8c) {
            f32 strength = light[0x58] == 5 ? 2.0f : rtlDistanceStrength(light, position);
            if (strength != 0.0f && light[0x58] != 7) {
                rtlInsertLight(light, data, strength);
            }
        }
    }

    for (i32 slot = 0; slot < 3; ++slot) {
        u8 *light = *reinterpret_cast<u8 **>(data->data + slot * 4);
        NUVEC *colour = reinterpret_cast<NUVEC *>(data->data + 0x78 + slot * sizeof(NUVEC));
        NUVEC *direction = reinterpret_cast<NUVEC *>(data->data + 0x9c + slot * sizeof(NUVEC));
        if (light == NULL) {
            *colour = {0.0f, 0.0f, 0.0f};
            *direction = {0.0f, 1.0f, 0.0f};
            continue;
        }
        // rtlCalcLights (original 0x3abcb8) resets the directional
        // light's selection priority before using it as intensity.
        if (light[0x58] == 5) {
            *reinterpret_cast<f32 *>(data->data + 0x0c + slot * 4) = 1.0f;
        }
        const f32 strength =
            *reinterpret_cast<f32 *>(data->data + 0x0c + slot * 4) * *reinterpret_cast<f32 *>(light + 0x6c) * scale;
        const NUVEC *source_colour = reinterpret_cast<const NUVEC *>(light + 0x18);
        NuVecScale(colour, const_cast<NUVEC *>(source_colour), strength);
        if (light[0x58] == 2 || light[0x58] == 3 || light[0x58] == 6 || light[0x58] == 8) {
            NuVecSub(direction, reinterpret_cast<NUVEC *>(light), position);
            NuVecNorm(direction, direction);
        } else if (light[0x58] == 4) {
            *direction = *reinterpret_cast<NUVEC *>(light + 0x0c);
        } else {
            *direction = {0.0f, 0.0f, 1.0f};
            NuVecRotateX(direction, direction, *reinterpret_cast<i16 *>(light + 0x5a));
            NuVecRotateY(direction, direction, *reinterpret_cast<i16 *>(light + 0x5c));
            NuVecMtxRotate(direction, direction, &global_camera.mtx);
        }
        if (rotation != NULL) {
            NuVecMtxRotate(direction, direction, rotation);
        }
    }

    NUVEC *ambient = reinterpret_cast<NUVEC *>(data->data + 0xc0);
    *ambient = {0.0f, 0.0f, 0.0f};
    for (i32 slot = 0; slot < 3; ++slot) {
        u8 *light = *reinterpret_cast<u8 **>(data->data + 0x18 + slot * 4);
        if (light == NULL) {
            continue;
        }
        const f32 strength =
            *reinterpret_cast<f32 *>(data->data + 0x24 + slot * 4) * *reinterpret_cast<f32 *>(light + 0x6c) * scale;
        const NUVEC *colour = reinterpret_cast<const NUVEC *>(light + 0x18);
        ambient->x = ClampUnit(ambient->x + colour->x * strength);
        ambient->y = ClampUnit(ambient->y + colour->y * strength);
        ambient->z = ClampUnit(ambient->z + colour->z * strength);
    }
}

extern "C" {
    void rtlApplySetScale(void *set, rtldata_s *data, NUVEC *position, NUMTX *rotation, i32 identity, f32 scale) {
        rtlidata_s local_data;
        rtlidata_s *lighting_data;

        _NuTimeBarSlotBegin(0, 6, "RTL srch");
        if (data == NULL) {
            lighting_data = &local_data;
            rtlResetEx(reinterpret_cast<rtldata_s *>(lighting_data), 1);
        } else {
            lighting_data = reinterpret_cast<rtlidata_s *>(data);
            rtlResetEx(reinterpret_cast<rtldata_s *>(lighting_data), 0);
            if (lighting_data->cached_light != NULL &&
                static_cast<u16>(lighting_data->cached_light->uid) != lighting_data->cached_light_uid) {
                lighting_data->cached_light = NULL;
                lighting_data->cached_light_uid = 0;
                lighting_data->cached_value = 0.0f;
            }
        }

        if (rtl_dynamic_pool != NULL && rtl_dynamic_lights_enabled != 0) {
            rtlApplySetScaleLoop(NULL, lighting_data, position, rotation, identity, scale);
        }
        if (set != NULL) {
            rtlApplySetScaleLoop(set, lighting_data, position, rotation, identity, scale);
        }
        rtlCalcLights(position, rotation, scale, lighting_data);
        rtlCalcShadow(lighting_data);
        _NuTimeBarSlotEnd(0, 6);
    }

    i32 rtlDynamicAlloc(void) {
        if (rtl_dynamic_pool == NULL)
            return -1;
        rtl_s *light = reinterpret_cast<rtl_s *>(NuLstAllocTail(rtl_dynamic_pool));
        if (light != NULL) {
            light->type = 4;
            NuVecClear(&light->position);
            light->inner_radius = 1.0f;
            light->outer_radius = 2.0f;
            light->colour.x = 1.0f;
            light->colour.y = 1.0f;
            light->colour.z = 1.0f;
            light->secondary_colour.x = 0.5f;
            light->secondary_colour.y = 0.5f;
            light->secondary_colour.z = 0.5f;
            light->type = 2;
            light->disabled = 0;
            light->field_5e = 0;
            light->field_60 = 0;
            light->parameters[0] = 0.1f;
            light->parameters[1] = 0.1f;
            light->parameters[2] = 0.1f;
            light->parameters[3] = 0.1f;
            light->parameter_54 = 0.0f;
            light->pitch = 0;
            light->yaw = 0;
            light->direction.x = 0.0f;
            light->direction.y = 0.0f;
            light->direction.z = 1.0f;
            light->field_64 = 0.0f;
            light->intensity = 1.0f;
            light->field_7a = -1;
            light->field_79 = -1;
            light->field_7b = 0;
            light->field_7c = 0;
            NuVecRotateX(&light->direction, &light->direction, light->pitch);
            NuVecRotateY(&light->direction, &light->direction, light->yaw);
            light->uid = rtl_uid++;
            if (rtl_uid == 0)
                ++rtl_uid;
            ++rtl_dynamic_cnt;
            NULNKHDR *header = reinterpret_cast<NULNKHDR *>(light) - 1;
            return header->id;
        }
        return -1;
    }

    i32 rtlDynamicAllocTemplate(rtlset *set, i32 user_id) {
        i32 id = -1;
        i32 template_id = rtlFindByUserId(reinterpret_cast<usize>(set), user_id);
        if (template_id >= 0) {
            id = rtlDynamicAlloc();
            if (id >= 0) {
                rtl_s *light = reinterpret_cast<rtl_s *>(NuLstGetByIdx(rtl_dynamic_pool, id));
                rtlset *source = set;
                *light = source->lights[template_id];
                return id;
            }
        }
        return -1;
    }

    void rtlDynamicFree(i32 id) {
        if (rtl_dynamic_pool != NULL && id >= 0 && id < rtl_dynamic_max) {
            NULNKHDR *light = NuLstGetByIdx(rtl_dynamic_pool, id);
            if (light != NULL) {
                NuLstFree(light);
                --rtl_dynamic_cnt;
            }
        }
    }

    void rtlDynamicMasterEnable(i32 enabled) {
    }

    bool rtlDynamicEnable(i32 id, i32 enabled) {
        if (rtl_dynamic_pool == NULL || id < 0 || id >= rtl_dynamic_max)
            return false;
        rtl_s *light = reinterpret_cast<rtl_s *>(NuLstGetByIdx(rtl_dynamic_pool, id));
        if (light == NULL)
            return false;
        bool previous = (light->flags & 1) == 0;
        light->flags = (light->flags & ~1) | (enabled == 0);
        return previous;
    }

    i32 rtlDynamicSetColours(i32 id, NUVEC *colour, NUVEC *secondary) {
        if (rtl_dynamic_pool == NULL || id < 0 || id >= rtl_dynamic_max)
            return 0;
        rtl_s *light = reinterpret_cast<rtl_s *>(NuLstGetByIdx(rtl_dynamic_pool, id));
        if (light == NULL || (colour == NULL && secondary == NULL))
            return 0;
        if (colour != NULL)
            light->colour = *colour;
        if (secondary != NULL)
            light->secondary_colour = *secondary;
        return 1;
    }

    void rtlDynamicSetDirection(void) {
    }

    i32 rtlDynamicSetPos(i32 id, NUVEC *position) {
        if (rtl_dynamic_pool == NULL || id < 0 || id >= rtl_dynamic_max)
            return 0;
        rtl_s *light = reinterpret_cast<rtl_s *>(NuLstGetByIdx(rtl_dynamic_pool, id));
        if (light == NULL || position == NULL)
            return 0;
        light->position = *position;
        return 1;
    }

    i32 rtlDynamicSetRadii(i32 id, f32 inner, f32 outer) {
        if (rtl_dynamic_pool == NULL || id < 0 || id >= rtl_dynamic_max)
            return 0;
        rtl_s *light = reinterpret_cast<rtl_s *>(NuLstGetByIdx(rtl_dynamic_pool, id));
        if (light == NULL)
            return 0;
        light->inner_radius = inner;
        if (outer < inner)
            outer = inner;
        light->outer_radius = outer;
        return 1;
    }

    i32 rtlDynamicSetType(i32 id, i32 type) {
        if (rtl_dynamic_pool == NULL || id < 0 || id >= rtl_dynamic_max || type <= 0 || type >= 9)
            return 0;
        rtl_s *light = reinterpret_cast<rtl_s *>(NuLstGetByIdx(rtl_dynamic_pool, id));
        if (light == NULL)
            return 0;
        light->type = type;
        return 1;
    }

    void rtlFrameUpdate(f32 frame_time) {
        rtltimer1 = static_cast<u16>(static_cast<i32>(rtltimer1adv * frame_time) + rtltimer1);
        NuTimeBarSlotReset(0, 6);
    }

    void rtlFree(void) {
    }

    void rtlGetCurrentSet(void) {
    }

    void rtlGetEnvPath(void) {
    }

    void rtlGetEnvSceneName(void) {
    }

    void rtlGetEnvSet(void) {
    }

    void rtlGetFogSet(void) {
    }

    i32 rtlInitDynamic(VARIPTR *buffer, VARIPTR end, i32 max_lights) {
        rtl_dynamic_pool = NuLstCreateBuff(max_lights, sizeof(rtl_s), buffer, end, 0x10);
        rtl_dynamic_max = max_lights;
        rtl_dynamic_cnt = 0;
        return max_lights;
    }

    rtlset *rtlLoadSet(char *path, VARIPTR *buffer, i32 buffer_end) {
        buffer->addr = ALIGN(buffer->addr, 4);
        rtlset *set = static_cast<rtlset *>(buffer->void_ptr);
        memset(set, 0, 0x4f84);

        if (NuFileLoadBuffer(path, set, buffer_end - buffer->addr) > 0) {
            rtlSwapSetEndianess(set);
        }

        u8 *bytes = reinterpret_cast<u8 *>(set);
        for (i32 i = 0; i < 0x80; ++i) {
            *reinterpret_cast<i16 *>(bytes + i * 0x8c + 0x6e) = static_cast<i16>(i + 1);
            *reinterpret_cast<rtlset **>(bytes + i * 0x8c + 0x80) = set;
            *reinterpret_cast<f32 *>(bytes + i * 0x8c + 0x70) = 1.0f;
        }
        *reinterpret_cast<u32 *>(bytes) = 5;
        buffer->addr += 0x4f84;
        IndexLights(set, buffer, buffer_end);
        return set;
    }

    void rtlProcessLights(void *, f32) {
    }

    void rtlReset(rtldata_s *data) {
        rtlResetEx(data, 0);
    }

    void rtlSaveSet(void) {
    }

    void rtlScaleSetMultipliers(void) {
    }

    void rtlSetAssocName(void) {
    }

    void rtlSetExt(void) {
    }

    void rtlSetLights(rtldata_s *data) {
        const NUVEC *directions = reinterpret_cast<const NUVEC *>(data->data + 0x9c);
        const NUCOLOUR3 *colours = reinterpret_cast<const NUCOLOUR3 *>(data->data + 0x78);
        NuRndrSetDirectionalLightsPS(&directions[0], &colours[0], &directions[1], &colours[1], &directions[2],
                                     &colours[2]);
        NuRndrSetAmbientLightPS(reinterpret_cast<const NUCOLOUR3 *>(data->data + 0xc0));
    }

    void rtlSetMinR(void) {
    }

    void rtlSetModifiers(void) {
    }

    void rtlSetShadowFlickerBlendTime(void) {
    }

    void rtlSetShadowFlickerScale(void) {
    }

    void rtlSetSpecularLight(void) {
    }

    void rtlSetSpecularValue(void) {
    }

    void rtlSetUndoBuffer(void) {
    }

    void rtlSetUserIdName(void) {
    }

    void rtlSpecularValue(void) {
    }

    void rtlResetDynamic(void) {
        if (rtl_dynamic_pool != NULL) {
            NULNKHDR *entry = NuLstGetNext(rtl_dynamic_pool, NULL);
            while (entry != NULL) {
                NULNKHDR *next = NuLstGetNext(rtl_dynamic_pool, entry);
                NuLstFree(entry);
                entry = next;
            }
            rtl_dynamic_cnt = 0;
        }
    }

    i32 rtlFindByUserId(usize rtl_set, i32 user_id) {
        if (rtl_set != 0) {
            rtlset *set = reinterpret_cast<rtlset *>(rtl_set);
            for (i32 i = 0; i < 128; ++i) {
                if (set->lights[i].type != 0 && set->lights[i].field_68 == user_id) {
                    return i;
                }
            }
        }
        return -1;
    }

    void rtlGetDirection(usize rtl_set, i32 id, void **out) {
        (void)rtl_set;
        (void)id;
        (void)out;
    }

} // extern "C"

void SelectNextRTL() {
}

void SelectPrevRTL() {
}

void rtlSwapSetEndianess(rtlset *) {
}
