#include "legoapi/world/world_shared.h"
#include "legoapi/legoapi_types.h"
#include "legoapi/items/base/apiobject.h"
#include "decomp.h"
#include "nu2api/numath/numtx.h"
#include "nu2api/numath/nufloat.h"
#include "nu2api/numath/nutrig.h"
#include "nu2api/nucore/nustring.h"
#include "nu2api/nu3d/android/nuiosdl_gl.h"
#include "nu2api/nu3d/nucamera.h"
#include "nu2api/nu3d/nudlist.h"
#include "nu2api/nu3d/nuspecial.h"
#include "nu2api/nu3d/nurndrstat.h"
#include "nu2api/nu3d/nuvport.h"
#include "globals.h"

#include <string.h>

static f32 EvaluateDebrisCurve(const debris_float_key_s (&keys)[8], f32 time) {
    for (i32 i = 0; i < 7; ++i) {
        if (keys[i].time <= time && time <= keys[i + 1].time) {
            const f32 duration = keys[i + 1].time - keys[i].time;
            if (time == keys[i].time || duration == 0.0f) {
                return keys[i].value;
            }
            const f32 t = (time - keys[i].time) / duration;
            return keys[i].value + (keys[i + 1].value - keys[i].value) * t;
        }
    }
    return 0.0f;
}

static u8 ClampDebrisColour(f32 value) {
    value += value;
    return static_cast<u8>(value > 255.0f ? 255.0f : value);
}

static u32 EvaluateDebrisColour(const debinftype *effect, f32 time) {
    f32 red = 0.0f;
    f32 green = 0.0f;
    f32 blue = 0.0f;
    for (i32 i = 0; i < 7; ++i) {
        const debris_colour_key_s &first = effect->colour_keys[i];
        const debris_colour_key_s &second = effect->colour_keys[i + 1];
        if (first.time <= time && time <= second.time) {
            const f32 duration = second.time - first.time;
            const f32 t = time == first.time || duration == 0.0f ? 0.0f : (time - first.time) / duration;
            red = first.red + (static_cast<i32>(second.red) - first.red) * t;
            green = first.green + (static_cast<i32>(second.green) - first.green) * t;
            blue = first.blue + (static_cast<i32>(second.blue) - first.blue) * t;
            break;
        }
    }
    const u8 alpha = static_cast<u8>(EvaluateDebrisCurve(effect->alpha_keys, time));
    return static_cast<u32>(alpha) << 24 | static_cast<u32>(ClampDebrisColour(red)) |
           static_cast<u32>(ClampDebrisColour(green)) << 8 | static_cast<u32>(ClampDebrisColour(blue)) << 16;
}

struct numtl_s;
typedef struct numtl_s NUMTL;

extern "C" {

    extern NUGLOBALRNDRSTATE render_state;
    extern nudisplayscene_s currentScene;

    void *NuVisiEvaluate(NUGSCN *scene, void *visibility_context);

    void clipRoomAgainstFrustrum(NUGSCN *scene, NUROOM *room, NUFRUSTRUM *frustum) {
        u32 instance_index;
        i32 clip_result;
        u32 i;

        // The original routine exits before selecting the bounds path when a
        // display list is unavailable; the sphere path also needs its flags
        // and override tables below.
        if (scene->display_list == NULL) {
            return;
        }

        if ((scene->display_list->render_buffer & NUDL_SCENE_RENDER_FLAG_CENTER_EXTENT_BOUNDS) != 0) {
            for (i = 0; i < static_cast<u32>(static_cast<i32>(room->instance_count)); ++i) {
                instance_index = room->instance_indices[i];
                u8 *instance_record = scene->instances + instance_index * 0x50;
                (void)instance_record;
                if ((((u8)PortalVisiFlags[instance_index >> 3] >> (instance_index & 7)) & 1) == 0) {
                    if ((((u8)scene->display_list->portal_visibility_overrides[instance_index >> 3] >>
                          (instance_index & 7)) &
                         1) == 0) {
                        if ((scene->display_list->visibility_flags[instance_index] & NUDL_INSTANCE_FLAG_VISIBLE) != 0) {
                            if ((scene->display_list->visibility_flags[instance_index] &
                                 NUDL_INSTANCE_FLAG_NO_VISIBILITY_TEST) != 0) {
                                PortalVisiFlags[instance_index >> 3] |= 1 << (instance_index & 7);
                            } else {
                                clip_result = NuPortalClipTestBox(&scene->portal_boxes[instance_index].first,
                                                                  &scene->portal_boxes[instance_index].second, frustum);
                                if (clip_result != 0) {
                                    PortalVisiFlags[instance_index >> 3] |= 1 << (instance_index & 7);
                                }
                            }
                        }
                    } else {
                        PortalVisiFlags[instance_index >> 3] |= 1 << (instance_index & 7);
                    }
                }
            }
        } else {
            for (i = 0; i < static_cast<u32>(static_cast<i32>(room->instance_count)); ++i) {
                instance_index = room->instance_indices[i];
                u8 *instance_record = scene->instances + instance_index * 0x50;
                (void)instance_record;
                if ((((u8)PortalVisiFlags[instance_index >> 3] >> (instance_index & 7)) & 1) == 0) {
                    if ((((u8)scene->display_list->portal_visibility_overrides[instance_index >> 3] >>
                          (instance_index & 7)) &
                         1) == 0) {
                        if ((scene->display_list->visibility_flags[instance_index] & NUDL_INSTANCE_FLAG_VISIBLE) != 0) {
                            if ((scene->display_list->visibility_flags[instance_index] &
                                 NUDL_INSTANCE_FLAG_NO_VISIBILITY_TEST) == 0) {
                                clip_result = clipTestSphere(&scene->portal_spheres[instance_index], frustum);
                                if (clip_result == 1) {
                                    PortalVisiFlags[instance_index >> 3] |= 1 << (instance_index & 7);
                                } else if (clip_result == 2 &&
                                           clipTestBox(&scene->portal_boxes[instance_index].first,
                                                       &scene->portal_boxes[instance_index].second, frustum->planes,
                                                       static_cast<i32>(frustum->plane_count)) != 0) {
                                    PortalVisiFlags[instance_index >> 3] |= 1 << (instance_index & 7);
                                }
                            } else {
                                PortalVisiFlags[instance_index >> 3] |= 1 << (instance_index & 7);
                            }
                        }
                    } else {
                        PortalVisiFlags[instance_index >> 3] |= 1 << (instance_index & 7);
                    }
                }
            }
        }
    }
    i32 NuDisplayListRndrSpecial(nuhspecial_s *special, NUMTX *matrix, i32 skinned, NUMTX *skin_matrices,
                                 DEFORMERWEIGHTSARRAY *deformer_weights);

    void AddColourPick(void) {
    }

    void ChooseCorrectLOD(void) {
    }

    void *DisplayListCreateFaceonTransformPS(VARIPTR *buffer, NUMTX *transform, NUMTL *, void *faceon_data) {
        buffer->addr = ALIGN(buffer->addr, 4);
        auto *packet = static_cast<NuFaceOnTransformPacket *>(buffer->void_ptr);
        buffer->addr += sizeof(*packet);

        packet->world = *transform;
        const f32 scale_squared = NuVecMagSqr(reinterpret_cast<NUVEC *>(&transform->m00)) +
                                  NuVecMagSqr(reinterpret_cast<NUVEC *>(&transform->m10)) +
                                  NuVecMagSqr(reinterpret_cast<NUVEC *>(&transform->m20));
        packet->magnitude = NuFsqrt(scale_squared / 3.0f);

        NUVEC direction = {};
        if (*static_cast<i32 *>(faceon_data) == 0) {
            NuMtxCalcCheapFaceOn(&packet->face_on, &direction);
        } else {
            direction.x = transform->m10;
            direction.y = transform->m11;
            direction.z = transform->m12;
            NuMtxCalcCheapFaceY_v2(&packet->face_on, &direction);
        }
        return packet;
    }

    void DisplayListCreateFxItemPS(void *, i32) {
    }

    void *DisplayListCreateGeomTransformPS(VARIPTR *buffer, NUMTX *transform, NUMTL *mtl, void *next, void *tx) {
        static NUMTX scale;
        static NUMTX translate;
        static NUMTX viewtranslate;
        static NUMTX viewscale;
        static NUMTX viewTransform;

        (void)mtl;
        (void)next;
        (void)tx;

        NUMTX *projection = NuCameraGetProjectionMtx();
        NUMTX *view = NuCameraGetViewMtx();
        (void)projection;
        (void)view;

        NUVEC scale_vector = {1.0f, 1.0f, 1.0f};
        NUVEC translation_vector = {0.0f, 0.0f, 0.0f};
        scale_vector.x = g_NuVpRegion.projection_x_scale;
        scale_vector.y = g_NuVpRegion.projection_y_scale;
        NuMtxSetScale(&scale, &scale_vector);
        translation_vector.x = g_NuVpRegion.projection_x_offset;
        translation_vector.y = g_NuVpRegion.projection_y_offset;
        NuMtxSetTranslation(&translate, &translation_vector);

        buffer->addr = ALIGN(buffer->addr, 4);
        NUMTX *result = static_cast<NUMTX *>(buffer->void_ptr);
        NuMtxTranspose(result, transform);
        buffer->addr += sizeof(NUMTX);
        return result;
    }

    void DisplayListCreatePS(void) {
    }

    void DisplayListCreateRigidSkin2TransformPS(void) {
    }

    void DisplayListCreateRigidSkinTransformPS(void) {
    }

    // Original 0x29b338.  Build the compact skin-palette packet consumed by
    // NuIOSDLSkinMtxCallback.  Blend-shape deltas are applied to a stream copy
    // of immediate geometry before the ordinary geometry callback runs.
    void *DisplayListCreateSkinTransformPS(VARIPTR *buffer, NUMTX *skin_matrices,
                                           DEFORMERWEIGHTSARRAY *deformer_weights, NUDISPLAYLISTGEOM *geometry,
                                           NUDISPLAYLISTGEOM **render_geometry) {
        if (deformer_weights != NULL && geometry->immediate != 0) {
            buffer->addr = ALIGN(buffer->addr, 0x20);
            NUDISPLAYLISTGEOM *copy = static_cast<NUDISPLAYLISTGEOM *>(buffer->void_ptr);
            *render_geometry = copy;
            buffer->addr += sizeof(*copy);
            *copy = *geometry;

            buffer->addr = ALIGN(buffer->addr, 0x20);
            copy->dynamic_vertex_data = buffer->void_ptr;
            const usize vertex_data_size = static_cast<usize>(geometry->vertex_stride) * geometry->vertex_count;
            buffer->addr += vertex_data_size;

            u8 *source = reinterpret_cast<u8 *>(static_cast<usize>(geometry->vertex_buffer)) +
                         static_cast<usize>(geometry->vertex_stride) * geometry->base_vertex;
            u8 *destination = static_cast<u8 *>(copy->dynamic_vertex_data);
            memmove(destination, source, vertex_data_size);

            for (i32 vertex = 0; vertex < geometry->vertex_count; ++vertex) {
                NUVEC *position = reinterpret_cast<NUVEC *>(destination);
                for (i32 deformer = 0; deformer < deformer_weights->count; ++deformer) {
                    NUVEC *offsets = geometry->deformer_vertex_offsets[deformer];
                    const f32 weight = deformer_weights->weights[deformer];
                    if (offsets != NULL && weight != 0.0f) {
                        position->x += offsets[vertex].x * weight;
                        position->y += offsets[vertex].y * weight;
                        position->z += offsets[vertex].z * weight;
                    }
                }
                destination += geometry->vertex_stride;
                source += geometry->vertex_stride;
            }
        } else {
            geometry->dynamic_vertex_data = NULL;
            *render_geometry = geometry;
        }

        i32 *matrix_count = static_cast<i32 *>(buffer->void_ptr);
        void *packet = matrix_count;
        *matrix_count = 0;
        buffer->addr += sizeof(*matrix_count);
        for (i32 i = 0; i < 8 && geometry->joint_indices[i] != 0xff; ++i) {
            NUMTX *packet_matrix = static_cast<NUMTX *>(buffer->void_ptr);
            *packet_matrix = skin_matrices[geometry->joint_indices[i]];
            buffer->addr += sizeof(NUMTX);
            ++*matrix_count;
        }
        return packet;
    }

    void DisplayListDebugPS(void) {
    }

    void DisplayListDestroyFxItemPS(void *) {
    }

    void NuDisplaySceneClonePS(NUDLDLISTSCENE *source, NUDLDLISTSCENE *destination, VARIPTR *buffer) {
        destination->ps = nullptr;
        if (source->ps) {
            void *copy = (void *)ALIGN(buffer->addr, 4);
            buffer->addr = ALIGN(buffer->addr, 4);
            buffer->addr += 4;
            memcpy(copy, source->ps, 4);
            destination->ps = copy;
        }
    }

    void DisplayListPrintItemPS(void) {
    }

    // DisplayListSetAlphaPS is fully transcribed in nu3d/nudlist.cpp (original 0x29b8c0 / 0x29b77e).
    void DisplayListSetFxItemParamPS(void *, i32, f32, i32) {
    }

    void DisplayListSetShadowCasterFlagPS(NUDISPLAYLISTITEM *previous, NUDISPLAYLISTITEM *, i32 flag) {
        NUMTX *transform = (NUMTX *)previous->next;
        transform->m32 = (f32)flag;
    }

    // DisplayListSwapBuffersPS is fully transcribed in nu3d/nudlist_stubs.cpp (original 0x29b8c0 / 0x29b77e).
    void *RndrStateBuildVertexGroupsStates(NURNDRSTATE *state);
    void DisplayListUpdateRenderStateShadow(NUDISPLAYLIST *list, NURNDRSTATE *state) {
        if (!state)
            return;
        if (list->state->global_id == state->global_id)
            return;
        if (list->state->vertex_groups_id != state->vertex_groups_id) {
            void *groups = RndrStateBuildVertexGroupsStates(state);
            if (groups)
                NuDisplayListLinkItem(list, 0xa9, groups);
            list->state->vertex_groups_id = state->vertex_groups_id;
        }
        list->state->global_id = state->global_id;
    }

    void DisplayListUpdateSpecialTransformPS(nuhspecial_s *, NUMTX *) {
    }

    void DisplaySceneRndrSpecials(NUDLDLISTSCENE *scene, i32, void *visibility_context) {
        NuVisibilityResult *visibility =
            static_cast<NuVisibilityResult *>(NuVisiEvaluate(scene->gscene, &visibility_context));
        const bool portal_filter = visibility != NULL && portal_special_objects != 0 &&
                                   visibility->portal_marker != NULL && portals_enabled != 0;
        const bool shadow_pass = currentScene.unknown_3c != NULL;

        if ((scene->instance_visibility_enabled & NUDL_SCENE_INSTANCE_VISIBILITY_ENABLED) != 0 ||
            noscenespecials != 0) {
            return;
        }

        NUGSCN temporary_scene;
        NUGSCN *gscene = scene->gscene;
        if (gscene == NULL) {
            temporary_scene.display_list = scene;
            gscene = &temporary_scene;
        }

        nuhspecial_s special_handle;
        special_handle.scene = gscene;
        for (i32 index = 0; index < scene->nspecials; ++index) {
            NUDISPLAYSPECIAL *special = &static_cast<NUDISPLAYSPECIAL *>(scene->specials)[index];
            if ((special->flags & NUDISPLAYSPECIAL_FLAG_VISIBLE) == 0) {
                continue;
            }

            const i32 instance_index = special->instance_ix;
            if (shadow_pass &&
                (static_cast<u8>(scene->visibility_flags[instance_index]) & NUDL_INSTANCE_FLAG_CASTS_SHADOW) == 0) {
                continue;
            }
            if (portal_filter &&
                (static_cast<u8>(scene->visibility_flags[instance_index]) & NUDL_INSTANCE_FLAG_NO_VISIBILITY_TEST) ==
                    0 &&
                (visibility->portal_bits[instance_index >> 3] & (1U << (instance_index & 7))) == 0) {
                continue;
            }

            special_handle.display_special = special;
            NUMTX *draw_matrix = special->draw_mtx_ptr;
            if (draw_matrix == NULL || draw_matrix == reinterpret_cast<NUMTX *>(-1)) {
                draw_matrix = &special->draw_mtx;
            }
            NuDisplayListRndrSpecial(&special_handle, draw_matrix, 0, NULL, NULL);
        }
    }

    void FmvTimePS(void) {
    }

    void GenericDebinfoDmaTypeUpdate(debinftype *effect) {
        extern PartHeader **DmaDebTypes;
        extern i32 EDPP_MAX_DMADEBTYPES;
        extern i32 freeDmaDebType;

        if (effect == NULL) {
            return;
        }
        if (effect->native_data == NULL) {
            if (freeDmaDebType >= EDPP_MAX_DMADEBTYPES) {
                return;
            }
            effect->native_data = DmaDebTypes[freeDmaDebType++];
        }

        if (NuStrCmp(effect->name, "FLY") == 0 && static_cast<u32>(effect->texture_u0) == 0x8003c &&
            static_cast<u32>(effect->texture_v0) == 0x80100 && static_cast<u32>(effect->texture_u1) == 0x8005e &&
            static_cast<u32>(effect->texture_v1) == 0x80082) {
            for (u32 i = 0; i < 8; ++i) {
                effect->width_keys[i].value = effect->height_keys[i].value;
                effect->height_keys[i].value += effect->height_keys[i].value;
                effect->alpha_keys[i].value *= 1.5f;
                effect->rotation_keys[i].value *= 1.5f;
            }
            effect->texture_u0 = static_cast<f32>(static_cast<u32>(effect->texture_u0) & ~0x1ffU) + 63.75f;
            effect->texture_v0 = static_cast<f32>(static_cast<u32>(effect->texture_v0) & ~0x1ffU) + 127.5f;
            effect->texture_u1 = static_cast<f32>(static_cast<u32>(effect->texture_u1) & ~0x1ffU) + 95.625f;
            effect->texture_v1 = static_cast<f32>(static_cast<u32>(effect->texture_v1) & ~0x1ffU) + 191.25f;
        }
        PartHeader *header = effect->native_data;
        header->gravity = effect->field_0a0;
        header->texture_u0 = static_cast<f32>(static_cast<i32>(effect->texture_u0) & 0x1ff) / 255.0f;
        header->texture_v0 = static_cast<f32>(static_cast<i32>(effect->texture_v0) & 0x1ff) / 255.0f;
        header->texture_u1 = static_cast<f32>(static_cast<i32>(effect->texture_u1) & 0x1ff) / 255.0f;
        header->texture_v1 = static_cast<f32>(static_cast<i32>(effect->texture_v1) & 0x1ff) / 255.0f;

        for (i32 frame_index = 0; frame_index < 64; ++frame_index) {
            const f32 time = static_cast<f32>(frame_index) / 64.0f;
            const f32 width = EvaluateDebrisCurve(effect->width_keys, time);
            const f32 height = EvaluateDebrisCurve(effect->height_keys, time);
            const f32 rotation = EvaluateDebrisCurve(effect->rotation_keys, time);
            const f32 sine = NU_SIN_LUT(rotation);
            const f32 cosine = NU_SIN_LUT(rotation + 16384.0f);
            const f32 wave_x = effect->field_0b4 * NU_SIN_LUT(effect->field_0b0 * time * 65536.0f);
            const f32 wave_y = effect->field_0bc * NU_SIN_LUT(effect->field_0b8 * time * 65536.0f);

            debris_particle_frame_s &frame = header->frames[frame_index];
            frame.position.x = (-cosine * (width * 0.25f) - sine * (height * 0.25f) + wave_x) / 2048.0f;
            frame.position.y = (sine * (width * 0.25f) - cosine * (height * 0.25f) + wave_y) / 2048.0f;
            frame.position.z = 0.0f;
            frame.texture_offset.x = (cosine * (width * 0.25f) - sine * (height * 0.25f) + wave_x) / 2048.0f;
            frame.texture_offset.y = (-sine * (width * 0.25f) - cosine * (height * 0.25f) + wave_y) / 2048.0f;
            frame.texture_offset.z = 0.0f;
            frame.extent.x = (cosine * (width * 0.25f) + sine * (height * 0.25f) + wave_x) / 2048.0f;
            frame.extent.y = (-sine * (width * 0.25f) + cosine * (height * 0.25f) + wave_y) / 2048.0f;
            frame.extent.z = 0.0f;
            frame.colour = EvaluateDebrisColour(effect, time);
        }
    }

    void Initialise_PS(NUGSCN *scene) {
        scene->instance_visibility_flags = PortalVisiFlags;
    }

    // Original @0x3cd56e. Build the hierarchy render-part list selected by
    // the caller's bit mask.
    i32 MakeLayerList_Index(CHARACTERMODEL_s *model, i16 *layers, u32 mask) {
        if (model == NULL) {
            return 0;
        }

        i32 count = 0;
        u32 layer_bit = 1;
        for (i32 layer = 0; layer <= 31 && layer < model->hierarchy->render_count; ++layer) {
            if ((mask & layer_bit) != 0) {
                *layers++ = static_cast<i16>(layer);
                ++count;
            }
            layer_bit <<= 1;
        }
        return count;
    }

    void PerspectMidPoint(NUVEC *result, NUVEC *first, NUVEC *second, NUVEC *camera_position) {
        f32 first_distance = NuVecDist(camera_position, first, NULL);
        f32 second_distance = NuVecDist(camera_position, second, NULL);
        f32 ratio = first_distance / (first_distance + second_distance);
        result->x = first->x + (second->x - first->x) * ratio;
        result->y = first->y + (second->y - first->y) * ratio;
        result->z = first->z + (second->z - first->z) * ratio;
    }

    void RndrMaskScreen(void) {
    }

    void *RndrStateBuildFogState(NUGLOBALRNDRSTATE *state) {
        VARIPTR *buffer = NuDisplayListGetBuffer();
        void *result = buffer->void_ptr;

        *buffer->u32_ptr = state->fog_enabled;
        buffer->u32_ptr++;
        if (state->fog_enabled != 0) {
            *buffer->u32_ptr = state->fog_rgba;
            buffer->u32_ptr++;
            *reinterpret_cast<f32 *>(buffer->u32_ptr) = state->fog_near;
            buffer->u32_ptr++;
            *reinterpret_cast<f32 *>(buffer->u32_ptr) = state->fog_far;
            buffer->u32_ptr++;
            *reinterpret_cast<f32 *>(buffer->u32_ptr) = state->fog_density;
            buffer->u32_ptr++;
        }
        return result;
    }

    NULIGHTSTATE *RndrStateBuildLightState(NUGLOBALRNDRSTATE *state) {
        i32 i;
        f32 alpha = 1.0f;
        VARIPTR *buffer = NuDisplayListGetBuffer();
        NULIGHTSTATE *packet = (NULIGHTSTATE *)buffer->void_ptr;
        buffer->u8_ptr += sizeof(NULIGHTSTATE);
        packet->ambient_intensity.r = state->ambient_intensity.r;
        packet->ambient_intensity.g = state->ambient_intensity.g;
        packet->ambient_intensity.b = state->ambient_intensity.b;
        packet->ambient_intensity.a = 1.0f;
        for (i = 0; i < 3; ++i) {
            packet->light_intensity[i].r = state->light_intensity[i].r;
            packet->light_intensity[i].g = state->light_intensity[i].g;
            packet->light_intensity[i].b = state->light_intensity[i].b;
            packet->light_intensity[i].a = 1.0f;
            packet->light_direction[i].x = state->light_direction[i].x;
            packet->light_direction[i].y = state->light_direction[i].y;
            packet->light_direction[i].z = state->light_direction[i].z;
            packet->light_direction[i].w = 1.0f;
        }
        packet->specular_mtx = state->specular_mtx;
        packet->specular_colour = state->specular_colour;
        packet->specular_intensity = render_state.specular_intensity;
        return packet;
    }

    NUSPECIALVERTEXSTATES *nuspecial_vertex_states;
    void *RndrStateBuildVertexGroupsStates(NURNDRSTATE *) {
        f32 *output;
        i8 *input;
        i32 i;
        i32 max_groups = 128;
        VARIPTR *buffer;
        i32 vectors;
        VARIPTR packet;
        if (nuspecial_vertex_states) {
            buffer = NuDisplayListGetBuffer();
            packet = *buffer;
            *buffer->u32_ptr++ = nuspecial_vertex_states->count;
            *buffer->u32_ptr++ = nuspecial_vertex_states->flags;
            output = buffer->f32_ptr;
            vectors = (nuspecial_vertex_states->count + 3) / 4;
            buffer->u8_ptr += vectors * 16;
            input = nuspecial_vertex_states->values;
            for (i = 0; i < nuspecial_vertex_states->count; ++i) {
                *output = (f32)*input;
                ++output;
                ++input;
            }
            return packet.void_ptr;
        }
        return nullptr;
    }

    void *RndrStateBuildVertexOffsetsStates(NURNDRSTATE *) {
        VARIPTR *buffer;
        VARIPTR packet;
        if (nuspecial_vertex_noffsets > 0) {
            buffer = NuDisplayListGetBuffer();
            packet = *buffer;
            *buffer->u32_ptr++ = nuspecial_vertex_noffsets;
            memmove(buffer->void_ptr, nuspecial_vertex_offsets.void_ptr, nuspecial_vertex_noffsets * sizeof(NUVEC4));
            buffer->u8_ptr += nuspecial_vertex_noffsets * sizeof(NUVEC4);
            return packet.void_ptr;
        }
        return nullptr;
    }

    static void RndrStateClear(NUGLOBALRNDRSTATE *state) {
        state->state.mtl = NULL;
        state->state.tex_id = -1;
        state->state.global_id = -1;
        state->state.lights_id = -1;
        state->state.camera_id = -1;
        state->state.fog_id = -1;
        state->state.konst_id = -1;
        state->state.reflection_id = -1;
    }

    void RndrStateCopyGlobalState(NUGLOBALRNDRSTATE *state) {
        memcpy(state, &render_state, sizeof(*state));
        state->fog_state = NULL;
        state->camera_state = NULL;
        state->light_state = NULL;
        state->konst_state = NULL;
    }

    void RndrStateResetGlobalState(NUGLOBALRNDRSTATE *state) {
        state->const_alpha_enabled = 0;
        state->const_tint_enabled = 0;
        state->fog_state = NULL;
        state->light_state = NULL;
        state->camera_state = NULL;
        state->konst_state = NULL;
        state->reflection_state = NULL;
        render_state.state.global_id = 0;
        render_state.state.lights_id = 0;
        render_state.state.camera_id = 0;
        render_state.state.fog_id = 0;
        render_state.state.konst_id = 0;
        RndrStateClear(&render_state);
    }

    void RndrStateResetSharedGlobalState(void) {
        RndrStateResetGlobalState(&render_state);
    }

    void RndrStateSetConstAlphaTint(i32 alpha_enabled, i32 tint_enabled, f32 alpha, const NUCOLOUR3 *tint, NUMTL *mtl) {
        render_state.const_alpha_mtl = mtl;
        render_state.const_alpha_enabled = alpha_enabled;
        render_state.const_alpha = alpha;
        if (tint != NULL && tint_enabled != 0) {
            render_state.const_tint = *tint;
        }
        render_state.const_tint_enabled = tint_enabled;
        render_state.konst_state = NULL;
        render_state.state.global_id++;
        render_state.state.konst_id++;
    }

    void RndrStateSetReflection(i32 reflection) {
        render_state.reflection = reflection;
        render_state.reflection_state = NULL;
        render_state.state.global_id++;
        render_state.state.reflection_id++;
    }

    void RndrStateUpdate(void *, NUMTL *, NUDISPLAYLISTITEM *) {
    }

    void RndrStateUpdateFx(void *, NUDISPLAYLISTITEM *) {
    }

    void SetAllInstancesVisible(NUGSCN *scene) {
        memset(PortalVisiFlags, 0xff, sizeof(PortalVisiFlags));
    }

} // extern "C"

void SetAllInstancesHidden(NUGSCN *) {
    memset(PortalVisiFlags, 0, sizeof(PortalVisiFlags));
}

extern "C" {

    i32 ShadowInfo(void) {
        extern TERRAIN_SHAPE *ShadPoly;
        return ShadPoly != NULL ? ShadPoly->material[0] : -1;
    }

} // extern "C"
