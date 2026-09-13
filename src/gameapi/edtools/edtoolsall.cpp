#include "gameapi_edtools_types.h"
#include "gameapi/edtools/edcam.h"
#include "gameapi/edtools/edfile.h"
#include "gameapi/edtools/edstubs.h"
#include "gameapi/edtools/edgra.h"
#include "nu2api/nu3d/nuspecial.h"
#include "nu2api/nu3d/nuspline.h"
#include "nu2api/nucore/nustring.h"
#include "nu2api/nucore/nuvideo.h"
#include "nu2api/numath/nuvec.h"
#include "nu2api/nufile/nufile.h"
#include <stdio.h>
#include <string.h>
#include "nu2api/numath/nurand.h"

EdRegistry theRegistry;
i32 pad_disabled;
eduimenu_s *edLevelPinnedMenu;

static NUGSPLINE *splineStore;
static i32 numSplinesLoaded;
char *EDSPLINE_FILECHECK = const_cast<char *>("EDSPLINE v. ");

extern "C" {
    extern edgra_clump_s *GrassClumps;
    extern i32 EDGRA_MAX_UNITS_PER_INDIVIDUAL_CLUMP;
    i32 edgra_nearest;
    i32 edgra_rotz, edgra_roty;
    NUVEC edgra_cam_pos;
    i32 edgra_nearest_instance;
    f32 edgra_size;
    i32 edgra_mode = 1;
    i32 edgra_clump_size;
    extern i32 edgra_units_used;
    extern void *edgra_free_vecbuffer;
    extern i32 EDGRA_MAX_CLUMPS, EDGRA_MAX_INDIVIDUAL_CLUMPS;
    extern i32 edgra_clumps_used, edgra_ind_clumps_used;
    extern i32 *IndGrassClumpsUsed;
    extern i32 edgra_page_used[8];
    extern NUGSCN *edgra_page_scene[8];
    extern void *edgra_page_terrain[8];
    extern NUMTX *edgra_page_matrix_stack[8];
    extern NUGSCN *edbits_base_scene;
    void *edbits_base_terrain;
    NUMTX *edgra_mtxbuffer;
    i32 edgra_copy_source = -1;
    i32 edgra_last_clump_in_buffer = -1;
    i32 edgra_pageid, edgra_instance_type;
    f32 edgra_global_fadein = 15.0f, edgra_global_fadeout = 25.0f;
    void edgraInitAllClumps(void);
    extern edpp_particle_s edpp_ptls[512];
    extern i32 edpp_nearest;
    extern NUVEC edpp_cam_pos;
    extern edanim_param_s AnimParams[64];
    extern NUGSCN *edanim_page_scene[8];
}

void EdTerrInit(void *, void *) {
}

void edDrawLine(nuvec_s *, nuvec_s *, unsigned char, unsigned char, unsigned char) {
}

void EdDrawBegin(i32) {
}

void edpartPlace(i32, nuvec_s *) {
}

void edppDoInput(nupad_s *) {
}

void EdTerrShadow(nuvec_s *, float, float, i32) {
}

void edbriDoInput(nupad_s *) {
}

void edgraDoInput(nupad_s *) {
}

void edpartCreate(nuvec_s *, i32) {
}

void edppPtlPlace(i32, nuvec_s *) {
}

void EdDrawPolyTri(VuVec const &, VuVec const &, VuVec const &, i32) {
}

void edanimDoInput(nupad_s *) {
}

void edbobsDrawBox(nuvec_s *, nuvec_s *, i32) {
}

void edbriFileSave(char *) {
}

void edgraFileSave(char *) {
}

void edpartDoInput(nupad_s *) {
}

void edppPtlCreate(nuvec_s *, i32) {
}

void edppPtlShelve(i32) {
}

void EdDrawLineCube(VuMtx const &, float, i32) {
}

void EdDrawPolyAxis(VuMtx const &, float, i32) {
}

void edanimFileSave(char *) {
}

void edpartInitType(i32) {
}

void edppDrawCursor() {
}

extern "C" {
    extern edpp_particle_s edpp_ptls[512];
    extern i32 edpp_instances_used;
    void DebFreeInstantly(i32 *);
}

void edppPtlDestroy(i32 index) {
    if (edpp_ptls[index].instance_id != -1) {
        if (edpp_ptls[index].instance_id != 99999)
            DebFreeInstantly(&edpp_ptls[index].instance_id);
        --edpp_instances_used;
        edpp_ptls[index].instance_id = -1;
    }
}

void EdDrawLineArrow(VuMtx const &, float, i32) {
}

void EdDrawLineCross(VuVec const &, float, i32) {
}

void EdDrawPolyArrow(VuVec const &, VuVec const &, i32, i32, float, float, float, float) {
}

void edbriDrawCursor() {
}

void edgraClumpPlace(i32 index, NUVEC *position) {
    edgra_clump_s *clump = &GrassClumps[index];
    clump->position = *position;
    clump->size = edgra_size;
    clump->rotation_z = edgra_rotz;
    clump->rotation_y = edgra_roty;
    if (edgra_mode != 3) {
        if (edgra_units_used + edgra_clump_size - clump->element_count <= 0x3000)
            clump->element_count = edgra_clump_size;
    }
    edgra_free_vecbuffer = static_cast<NUVEC *>(clump->vector_buffer) + clump->element_count;
    edgraInitAllClumps();
}

void edgraDrawCursor() {
}

void edpartPtlShelve(i32) {
}

void edpartScaleType(i32, float) {
}

void edppSaveEffects(char *, char) {
}

void EdDrawLineSphere(VuVec const &, float, float, i32) {
}

void EdDrawPolySector(VuVec const &, float, i32, i32, i32, i32, i32) {
}

void edanimDrawCursor() {
}

i32 edgraClumpCreate(NUVEC *position) {
    if (edgra_clumps_used == EDGRA_MAX_CLUMPS)
        return -1;
    if (edgra_copy_source == -1 && edgra_mode == 3) {
        if (edgra_units_used == 0x3000 || edgra_ind_clumps_used == EDGRA_MAX_INDIVIDUAL_CLUMPS)
            return -1;
    } else if (edgra_units_used + edgra_clump_size > 0x3000)
        return -1;
    i32 index = 0;
    while (GrassClumps[index].element_count != 0)
        ++index;
    edgra_clump_s *clump = &GrassClumps[index];
    if (edgra_copy_source != -1) {
        clump->field_18 = GrassClumps[edgra_copy_source].field_18;
        clump->special_index = GrassClumps[edgra_copy_source].special_index;
        clump->field_20 = GrassClumps[edgra_copy_source].field_20;
        clump->element_count = edgra_clump_size;
        clump->flags = GrassClumps[edgra_copy_source].flags;
        clump->page = edgra_pageid;
        clump->seed = NuRand(NULL);
        GrassClumps[index].unknown_25 = GrassClumps[edgra_copy_source].unknown_25;
        GrassClumps[index].field_2c = GrassClumps[edgra_copy_source].field_2c;
        GrassClumps[index].unknown_26 = GrassClumps[edgra_copy_source].unknown_26;
        GrassClumps[index].field_30 = GrassClumps[edgra_copy_source].field_30;
        GrassClumps[index].kind = GrassClumps[edgra_copy_source].kind;
        GrassClumps[index].near_distance = GrassClumps[edgra_copy_source].near_distance;
        GrassClumps[index].field_42 = GrassClumps[edgra_copy_source].field_42;
        GrassClumps[index].far_distance = GrassClumps[edgra_copy_source].far_distance;
        GrassClumps[index].field_44 = GrassClumps[edgra_copy_source].field_44;
        GrassClumps[index].field_43 = GrassClumps[edgra_copy_source].field_43;
    } else {
        clump->special_index = edgra_instance_type;
        clump->element_count = edgra_mode == 3 ? 1 : edgra_clump_size;
        clump->field_18 = 0.2f;
        clump->field_20 = 1.0f;
        clump->flags = 1;
        clump->page = edgra_pageid;
        clump->seed = NuRand(NULL);
        GrassClumps[index].unknown_25 = 1;
        GrassClumps[index].unknown_26 = 1;
        GrassClumps[index].field_2c = 0.0f;
        GrassClumps[index].field_30 = 1.0f;
        GrassClumps[index].kind = edgra_mode;
        GrassClumps[index].near_distance = edgra_global_fadein;
        GrassClumps[index].far_distance = edgra_global_fadeout;
        GrassClumps[index].field_42 = 1;
        GrassClumps[index].field_44 = 0.0f;
        GrassClumps[index].field_43 = 1;
    }
    if (GrassClumps[index].kind == 3) {
        i32 individual = 0;
        while (IndGrassClumpsUsed[individual])
            ++individual;
        GrassClumps[index].individual_index = individual;
        IndGrassClumpsUsed[individual] = 1;
        GetIndGrassClump(individual, 0)->position.x = 0.0f;
        GetIndGrassClump(individual, 0)->position.y = 0.0f;
        GetIndGrassClump(individual, 0)->position.z = 0.0f;
        GetIndGrassClump(individual, 0)->field_0c = 1.0f;
        GetIndGrassClump(individual, 0)->field_10 = edgra_rotz;
        GetIndGrassClump(individual, 0)->field_12 = edgra_roty;
        ++edgra_ind_clumps_used;
    } else
        GrassClumps[index].individual_index = -1;
    ++edgra_clumps_used;
    if (!edgra_page_used[edgra_pageid]) {
        edgra_page_used[edgra_pageid] = 1;
        edgra_page_scene[edgra_pageid] = edbits_base_scene;
        edgra_page_terrain[edgra_pageid] = edbits_base_terrain;
        edgra_page_matrix_stack[edgra_pageid] = edgra_mtxbuffer;
    }
    GrassClumps[index].vector_buffer = edgra_free_vecbuffer;
    edgra_last_clump_in_buffer = index;
    edgraClumpPlace(index, position);
    return index;
}

void edpartDrawCursor() {
}

void EdDrawLineCircleX(VuVec const &, float, i32, i32) {
}

void EdDrawLineCircleY(VuVec const &, float, i32, i32) {
}

void EdDrawLineCircleZ(VuVec const &, float, i32, i32) {
}

void EdDrawLineSegment(VuVec const &, VuVec const &, i32) {
}

void edanimParamCreate(i32) {
}

void edpartSaveEffects(char *, char) {
}

void edppPtlChangeType(i32, i32) {
}

void edppPtlCreateCopy(nuvec_s *, i32) {
}

void EdDrawPolyCylinder(VuMtx const &, float, float, float, i32, i32, i32, i32) {
}

void EdDrawPolyCylinder(VuVec const &, VuVec const &, i32, i32, i32, float, float, float) {
}

void edanimParamDestroy(i32) {
}

void edbitsDoSingleDump(i32 face) {
    char filename[32];
    i32 index;
    for (index = 0; index < 1000; ++index) {
        sprintf(filename, "pictures\\cub%03d_%d.bmp", index, face);
        if (NuFileSize(filename) <= 0)
            break;
        if (index == 999)
            break;
    }
    sprintf(filename, "pictures\\cub%03d_", index);
    NuPs2VideoScreenDump(filename, 1, 1.0f, 1.0f, face, 0, 0);
}

void edgraCalculatePage(char, i32) {
}

void edgraInstancePlace(i32 index, NUVEC *position) {
    GetIndGrassClump(GrassClumps[edgra_nearest].individual_index, index)->position.x =
        position->x - GrassClumps[edgra_nearest].position.x;
    GetIndGrassClump(GrassClumps[edgra_nearest].individual_index, index)->position.y =
        position->y - GrassClumps[edgra_nearest].position.y;
    GetIndGrassClump(GrassClumps[edgra_nearest].individual_index, index)->position.z =
        position->z - GrassClumps[edgra_nearest].position.z;
    GetIndGrassClump(GrassClumps[edgra_nearest].individual_index, index)->field_0c = 1.0f;
    GetIndGrassClump(GrassClumps[edgra_nearest].individual_index, index)->field_10 = edgra_rotz;
    GetIndGrassClump(GrassClumps[edgra_nearest].individual_index, index)->field_12 = edgra_roty;
    edgraInitAllClumps();
}

void edpartLookupObject(char *) {
}

void edSpline_FindAllBeg(nugscn_s *, char *, nugspline_s **, i32) {
}

void edSpline_FindAllSub(nugscn_s *, char *, nugspline_s **, i32) {
}

i32 LoadEditorSplines(char *path, VARIPTR *buf, VARIPTR *buf_end) {
    splineStore = reinterpret_cast<NUGSPLINE *>(buf->void_ptr);
    buf->addr = (buf->addr + 3) & ~static_cast<usize>(3);

    EdFileSetMedia(1);
    if (EdFileOpen(path, NUFILE_READ) == 0) {
        splineStore = NULL;
        return 0;
    }

    const i32 file_check_length = NuStrLen(EDSPLINE_FILECHECK);
    for (i32 i = 0; i < file_check_length; ++i) {
        if (EdFileReadChar() != EDSPLINE_FILECHECK[i]) {
            EdFileClose();
            splineStore = NULL;
            return 0;
        }
    }

    EdFileReadInt();
    i32 spline_count = EdFileReadInt();
    i32 point_count = EdFileReadInt();
    i32 string_bytes = EdFileReadInt();

    char *name_cursor = reinterpret_cast<char *>(splineStore + spline_count);
    char *name_end = name_cursor + string_bytes;
    NUVEC *point_cursor = reinterpret_cast<NUVEC *>((reinterpret_cast<usize>(name_end) + 3) & ~static_cast<usize>(3));
    NUVEC *data_end = point_cursor + point_count;
    if (reinterpret_cast<usize>(data_end) > buf_end->addr) {
        EdFileClose();
        splineStore = NULL;
        return 0;
    }

    for (i32 i = 0; i < spline_count; ++i) {
        EdFileReadChar();
        i32 name_length = EdFileReadInt();
        NUGSPLINE *spline = &splineStore[i];
        spline->name = name_cursor;
        spline->pt_size = sizeof(NUVEC);
        spline->length = static_cast<i16>(EdFileReadInt());
        spline->pts = point_cursor;

        for (i32 j = 0; j < name_length; ++j) {
            spline->name[j] = EdFileReadChar();
        }
        name_cursor += name_length;
        if (name_cursor > name_end) {
            EdFileClose();
            splineStore = NULL;
            EdFileClose();
            numSplinesLoaded = spline_count;
            buf->void_ptr = data_end;
            return 0;
        }

        for (i32 j = 0; j < spline->length; ++j) {
            point_cursor[j].x = EdFileReadFloat();
            point_cursor[j].y = EdFileReadFloat();
            point_cursor[j].z = EdFileReadFloat();
        }
        point_cursor += spline->length;
        if (point_cursor > data_end) {
            EdFileClose();
            splineStore = NULL;
            EdFileClose();
            numSplinesLoaded = spline_count;
            buf->void_ptr = data_end;
            return 0;
        }
    }

    EdFileClose();
    numSplinesLoaded = spline_count;
    buf->void_ptr = data_end;
    return spline_count;
}

NUGSPLINE *edSpline_SplineFind(NUGSCN *scene, char *name) {
    if (splineStore != NULL) {
        for (i32 i = 0; i < numSplinesLoaded; ++i) {
            if (NuStrICmp(splineStore[i].name, name) == 0) {
                return &splineStore[i];
            }
        }
    }
    return NuSplineFind(scene, name);
}

void edSpline_SplineList(nugscn_s *) {
}

void edanimParticlePlace(i32, nuvec_s *) {
}

void edanimStartAllPages() {
}

void edgraInstanceCreate(NUVEC *position) {
    if (edgra_nearest != -1 && GrassClumps[edgra_nearest].element_count != EDGRA_MAX_UNITS_PER_INDIVIDUAL_CLUMP) {
        i32 index = GrassClumps[edgra_nearest].element_count++;
        edgraInstancePlace(index, position);
    }
}

void edpartPtlChangeType(i32, i32) {
}

void edppDestroyAllPages() {
}

void edanimParticleCreate(nuvec_s *) {
}

void edgraInstanceDestroy(i32 index) {
    if (edgra_nearest != -1 && GrassClumps[edgra_nearest].element_count != 1) {
        if (index != GrassClumps[edgra_nearest].element_count - 1) {
            for (i32 i = index; i < GrassClumps[edgra_nearest].element_count - 1; ++i) {
                GetIndGrassClump(GrassClumps[edgra_nearest].individual_index, i)->position =
                    GetIndGrassClump(GrassClumps[edgra_nearest].individual_index, i + 1)->position;
                GetIndGrassClump(GrassClumps[edgra_nearest].individual_index, i)->field_0c =
                    GetIndGrassClump(GrassClumps[edgra_nearest].individual_index, i + 1)->field_0c;
                GetIndGrassClump(GrassClumps[edgra_nearest].individual_index, i)->field_10 =
                    GetIndGrassClump(GrassClumps[edgra_nearest].individual_index, i + 1)->field_10;
                GetIndGrassClump(GrassClumps[edgra_nearest].individual_index, i)->field_12 =
                    GetIndGrassClump(GrassClumps[edgra_nearest].individual_index, i + 1)->field_12;
            }
        }
        --GrassClumps[edgra_nearest].element_count;
        edgraInitAllClumps();
    }
}

void edppDetermineNearest(float max_distance_squared) {
    NUVEC delta;
    if (edpp_nearest != -1) {
        edpp_particle_s *particle = &edpp_ptls[edpp_nearest];
        if (particle->instance_id != 99999 && particle->instance_id != -1) {
            NuVecSub(&delta, &edpp_cam_pos, &particle->position);
            if (delta.x * delta.x + delta.y * delta.y + delta.z * delta.z == 0.0f) {
                return;
            }
        }
    }
    edpp_nearest = -1;
    for (i32 i = 0; i < 512; ++i) {
        if (edpp_ptls[i].instance_id == -1 || edpp_ptls[i].instance_id == 99999) {
            continue;
        }
        NuVecSub(&delta, &edpp_cam_pos, &edpp_ptls[i].position);
        float distance_squared = delta.x * delta.x + delta.y * delta.y + delta.z * delta.z;
        if (max_distance_squared < 0.0f || max_distance_squared > distance_squared) {
            max_distance_squared = distance_squared;
            edpp_nearest = i;
        }
    }
}

void edppHighlightNearest() {
}

void edppMultipleCopyCopy() {
}

void edbriDetermineNearest(float) {
}

void edgraSortVectorBuffer(i32 index) {
    NUVEC temporary[256];
    if (index != -1 && index != edgra_last_clump_in_buffer && GrassClumps[index].vector_buffer) {
        u8 *buffer = static_cast<u8 *>(GrassClumps[index].vector_buffer);
        usize bytes = GrassClumps[index].element_count * sizeof(NUVEC);
        memcpy(temporary, buffer, bytes);
        memmove(buffer, buffer + bytes, static_cast<u8 *>(edgra_free_vecbuffer) - (buffer + bytes));
        memcpy(static_cast<u8 *>(edgra_free_vecbuffer) - bytes, temporary, bytes);
        edgra_last_clump_in_buffer = index;
        for (i32 i = 0; i < EDGRA_MAX_CLUMPS; ++i) {
            if (i != index && GrassClumps[i].element_count &&
                GrassClumps[i].vector_buffer > GrassClumps[index].vector_buffer) {
                GrassClumps[i].vector_buffer = static_cast<u8 *>(GrassClumps[i].vector_buffer) - bytes;
            }
        }
        GrassClumps[index].vector_buffer = static_cast<u8 *>(edgra_free_vecbuffer) - bytes;
        edgraInitAllClumps();
    }
}

void edpartDestroyAllPages() {
}

void edppMultipleCopyClear() {
}

void edppMultipleCopyPaste() {
}

void edppStartSingleEffect(i32) {
}

void edpartHighlightNearest() {
}

void edpartMultipleCopyCopy() {
}

void edpartMultipleCopyClear() {
}

float edanimPlayerAnimDistance(i32 parameter_index) {
    if (edmainQueryLocVec() != NULL) {
        nuhspecial_s special;
        NuGScnGetSpecial(&special, edanim_page_scene[AnimParams[parameter_index].page],
                         AnimParams[parameter_index].instance_id);
        NUVEC *position = edmainQueryLocVec();
        return NuVecDist(NuSpecialGetPos(&special), position, NULL);
    }
    return 0.0f;
}

void edanimRenderSoundEmitters(i32) {
}

void edbobs_DrawCoordinateInfo(nuvec_s *, i32, i32) {
}

void edanimDetermineNearestAnim(float) {
}

void edgraDetermineNearestClump(f32 distance) {
    NUVEC delta;
    if (edgra_nearest != -1) {
        NuVecSub(&delta, &edgra_cam_pos, &GrassClumps[edgra_nearest].position);
        if (delta.x * delta.x + delta.y * delta.y + delta.z * delta.z == 0.0f)
            return;
    }
    edgra_nearest = -1;
    for (i32 i = 0; i < EDGRA_MAX_CLUMPS; ++i) {
        if (GrassClumps[i].element_count) {
            NuVecSub(&delta, &edgra_cam_pos, &GrassClumps[i].position);
            f32 candidate = delta.x * delta.x + delta.y * delta.y + delta.z * delta.z;
            if (distance < 0.0f || candidate < distance) {
                distance = candidate;
                edgra_nearest = i;
            }
        }
    }
    if (edgra_nearest != -1)
        edgraSortVectorBuffer(edgra_nearest);
}

void eduiItemFileSelectorCreate(u32, eduiiattr_s *, void (*)(eduimenu_s *, eduiitem_s *, u32), char *) {
}

void edanimDetermineNearestSound(float) {
}

void edanimRenderParticleEmitters(i32) {
}

void edgraDetermineNearestInstance(f32 distance) {
    NUVEC delta;
    if (edgra_nearest == -1) {
        edgra_nearest_instance = -1;
        return;
    }
    i32 individual = GrassClumps[edgra_nearest].individual_index;
    if (edgra_nearest_instance != -1) {
        NuVecAdd(&delta, &GrassClumps[edgra_nearest].position,
                 &GetIndGrassClump(individual, edgra_nearest_instance)->position);
        NuVecSub(&delta, &edgra_cam_pos, &delta);
        if (delta.x * delta.x + delta.y * delta.y + delta.z * delta.z == 0.0f)
            return;
    }
    edgra_nearest_instance = -1;
    for (i32 i = 0; i < GrassClumps[edgra_nearest].element_count; ++i) {
        NuVecAdd(&delta, &GrassClumps[edgra_nearest].position, &GetIndGrassClump(individual, i)->position);
        NuVecSub(&delta, &edgra_cam_pos, &delta);
        f32 candidate = delta.x * delta.x + delta.y * delta.y + delta.z * delta.z;
        if (distance < 0.0f || candidate < distance) {
            distance = candidate;
            edgra_nearest_instance = i;
        }
    }
}

void edanimDetermineNearestParticle(float) {
}

void EdDrawEnd() {
}

void EdDrawMtx(VuMtx const *) {
}

void EdTerrRay(VuVec &, VuVec &) {
}

EdManScale::EdManScale() {
}

void EdManScale::Process(EdInputContext &, ClassObjectList &) {
}

void EdManScale::Render(ClassObjectList &) {
}

void EdRegistry::AddMapping(char *, char *) {
}

void EdRegistry::AddObjectNotifier(EdObjectNotifier *) {
}

void EdRegistry::ClassIFaceProcess(EdClass *, void *, EdInputContext &) {
}

void EdRegistry::ClassIFaceProcess(i32, void *, EdInputContext &) {
}

void EdRegistry::ClassIFaceRender(EdClass *, void *, i32) {
}

void EdRegistry::ClassIFaceRender(i32, void *, i32) {
}

void EdRegistry::CreateObject(EdClassInterface *, void *, i32, i32, i32) {
}

void EdRegistry::DefunctObject(EdClassInterface *, void *, i32, i32) {
}

void EdRegistry::DestroyObject(EdClassInterface *, void *, i32, i32) {
}

void EdRegistry::GetClass(char *) {
}

void EdRegistry::GetClassId(char *) {
}

void EdRegistry::GetStreamClassMapping(EdStream &, i32 *, i32 &, i32) {
}

void EdRegistry::GetType(char *) {
}

void EdRegistry::GetTypeId(char *) {
}

void EdRegistry::Initialise(variptr_u &, variptr_u &, i32, i32, i32, i32) {
}

void EdRegistry::MapName(char *) {
}

void EdRegistry::NotifyCreateObject(void *, EdClass *, void *, i32, i32, i32) {
}

void EdRegistry::NotifyDefunctObject(void *, EdClass *, i32) {
}

void EdRegistry::NotifyDestroyObject(void *, EdClass *, i32, i32) {
}

void EdRegistry::NotifyReviveObject(void *, EdClass *, i32) {
}

void EdRegistry::RegisterBaseTypes() {
}

void EdRegistry::RegisterClass(char *, EdClassInterface *, i32) {
}

void EdRegistry::RegisterType(char *, i32, void (*)(EdStream &, void *, i32)) {
}

void EdRegistry::Serialise(EdStream &) {
}

void EdRegistry::SerialiseObjects(EdStream &, EdRegistry *) {
}

EdManRotate::EdManRotate() {
}

void EdManRotate::Process(EdInputContext &, ClassObjectList &) {
}

void EdManRotate::Render(ClassObjectList &) {
}

void EdManRotate::RotateItem(EdInputContext &, ClassObjectList &, i32, i32) {
}

void EdRefSpline::GetMemberData(void *, i32, void *, i32) {
}

void EdRefSpline::SetMemberData(void *, i32, void *, i32, i16 *) {
}

void EdBitControl::AddMenuItem(eduimenu_s *, EdRef *, void *) {
}

void EdBitControl::Refresh() {
}

void EdBitControl::cbButton(eduimenu_s *, eduiitem_s *, u32) {
}

void EdBitControl::cbChanged(eduimenu_s *, eduiitem_s *, u32) {
}

void EdBitControl::cbSelectItem(eduimenu_s *, eduiitem_s *, u32) {
}

void EdDefunctList::ReviveAll(i32) {
}

void EdEnumControl::AddMenuItem(eduimenu_s *, EdRef *, void *) {
}

void EdEnumControl::GetEnumString(i32) {
}

void EdEnumControl::GetEnumValue(char *) {
}

void EdEnumControl::Refresh() {
}

void EdEnumControl::cbButton(eduimenu_s *, eduiitem_s *, u32) {
}

void EdEnumControl::cbChanged(eduimenu_s *, eduiitem_s *, u32) {
}

void EdEnumControl::cbSelectItem(eduimenu_s *, eduiitem_s *, u32) {
}

void EdInputStream::SerialiseString(char **) {
}

void EdInputStream::SerialiseString(char **, i32) {
}

void EdInputStream::SerialiseString(char *, i32) {
}

void EdManipulator::DrawAxis(VuVec &, VuMtx *) {
}

void EdManipulator::DrawRotator(VuVec &) {
}

void EdManipulator::GetAxisLocators(VuVec &, VuVec *, VuMtx *) {
}

void EdManipulator::Process(EdInputContext &, ClassObjectList &) {
}

void EdManipulator::Render(ClassObjectList &) {
}

void EdManipulator::SelectAxis(EdInputContext &, VuVec &, VuVec &, VuVec &, VuMtx *) {
}

void EdManipulator::SelectRotator(EdInputContext &, VuVec &, VuVec &) {
}

void EdInputContext::Clear(i32 input) {
    if (static_cast<u32>(input) < 40) {
        values[input] = 0.0f;
        cleared[input] = 1;
    }
}

EdInputContext::EdInputContext() {
}

f32 EdInputContext::Get(i32 input) {
    if (static_cast<u32>(input) < 40) {
        return values[input];
    }
    return 0.0f;
}

f32 EdInputContext::GetHold(i32 input) {
    if (static_cast<u32>(input) < 40 && held[input] != 0) {
        return values[input];
    }
    return 0.0f;
}

f32 EdInputContext::GetPress(i32 input) {
    if (static_cast<u32>(input) < 40 && pressed[input] != 0) {
        return values[input];
    }
    return 0.0f;
}

void EdInputContext::Set(i32 input, float value, float repeat_delay) {
    if (value != 0.0f) {
        float now = current_time;
        float repeat_threshold = repeat_window + now;
        values[input] = value;
        float next_repeat = repeat_times[input];
        pressed[input] = held[input] == 0;
        held[input] = 1;
        if (next_repeat >= repeat_threshold || next_repeat == 0.0f) {
            repeated[input] = 1;
        }
        repeat_times[input] = now + repeat_delay;
        return;
    }

    values[input] = value;
    released[input] = held[input] != 0;
    repeat_times[input] = 0.0f;
    held[input] = 0;
}

void EdInputContext::Update(nucamera_s *, nupad_s *, float, bool) {
}

void EdOutputStream::SerialiseString(char **) {
}

void EdOutputStream::SerialiseString(char **, i32) {
}

void EdOutputStream::SerialiseString(char *, i32) {
}

void EdRefPlaceable::GetMemberData(void *, i32, void *, i32) {
}

void EdRefPlaceable::SetMemberData(void *, i32, void *, i32, i16 *) {
}

void EdColourControl::AddMenuItem(eduimenu_s *, EdRef *, void *) {
}

EdColourControl::EdColourControl() {
}

void EdColourControl::Refresh() {
}

void EdColourControl::cbButton(eduimenu_s *, eduiitem_s *, u32) {
}

void EdColourControl::cbChanged(eduimenu_s *, eduiitem_s *, u32) {
}

void EdColourControl::cbColourSelected(eduimenu_s *, eduiitem_s *, u32) {
}

void EdMatrixControl::AddMenuItem(eduimenu_s *, EdRef *, void *) {
}

void EdMatrixControl::Destroy() {
}

EdMatrixControl::EdMatrixControl() {
}

void EdMatrixControl::Refresh() {
}

void EdMatrixControl::SetMenuItemAttr(i32, eduiitem_s *, eduiiattr_s *, eduiiattr_s *) {
}

void EdMatrixControl::cbButton(eduimenu_s *, eduiitem_s *, u32) {
}

void EdMatrixControl::cbChanged(eduimenu_s *, eduiitem_s *, u32) {
}

void EdMatrixControl::cbSelected(eduimenu_s *, eduiitem_s *, u32) {
}

void EdStringControl::AddMenuItem(eduimenu_s *, EdRef *, void *) {
}

EdStringControl::EdStringControl() {
}

void EdStringControl::GetVal(char *, i32) {
}

void EdStringControl::Refresh() {
}

void EdStringControl::SetVal(char const *) {
}

void EdStringControl::cbChanged(eduimenu_s *, eduiitem_s *, u32) {
}

void EdStringControl::cbPress(eduimenu_s *, eduiitem_s *, u32) {
}

void EdVectorControl::AddMenuItem(eduimenu_s *, EdRef *, void *) {
}

void EdVectorControl::Destroy() {
}

EdVectorControl::EdVectorControl() {
}

void EdVectorControl::Refresh() {
}

void EdVectorControl::cbButton(eduimenu_s *, eduiitem_s *, u32) {
}

void EdVectorControl::cbChanged(eduimenu_s *, eduiitem_s *, u32) {
}

void EdVectorControl::cbSelected(eduimenu_s *, eduiitem_s *, u32) {
}

void EdClassInterface::DistanceToObject(VuVec &, VuVec &, void *, EdRef **) {
}

void EdClassInterface::DistanceToObject(VuVec &, void *, EdRef **) {
}

void EdClassInterface::GetNextObject(void *, i32 (*)(void *)) {
}

void EdSfxNameControl::AddMenuItem(eduimenu_s *, EdRef *, void *) {
}

EdSfxNameControl::EdSfxNameControl() {
}

void EdSfxNameControl::cbButton(eduimenu_s *, eduiitem_s *, u32) {
}

void EdSfxNameControl::cbChanged(eduimenu_s *, eduiitem_s *, u32) {
}

void EdSfxNameControl::cbSelectSfx(eduimenu_s *, eduiitem_s *, u32) {
}

void EdFileInputStream::BeginBlock(char const *) {
}

void EdFileInputStream::Eat(i32, i32) {
}

void EdFileInputStream::EndBlock() {
}

void EdFileInputStream::Open(i32, i32) {
}

void EdFileInputStream::SerialiseBuffer(void *, i32, i32) {
}

void EdFileOutputStream::BeginBlock(char const *) {
}

void EdFileOutputStream::Eat(i32, i32) {
}

void EdFileOutputStream::EndBlock() {
}

void EdFileOutputStream::Open(i32, i32) {
}

void EdFileOutputStream::SerialiseBuffer(void *, i32, i32) {
}

void EdRefSpecialObject::GetMemberData(void *, i32, void *, i32) {
}

void EdRefSpecialObject::SetMemberData(void *, i32, void *, i32, i16 *) {
}

EdSpecialObjectControl::EdSpecialObjectControl() {
}

void EdSpecialObjectControl::cbButton(eduimenu_s *, eduiitem_s *, u32) {
}

void EdSpecialObjectControl::cbChanged(eduimenu_s *, eduiitem_s *, u32) {
}

void EdSpecialObjectControl::cbSelectObject(eduimenu_s *, eduiitem_s *, u32) {
}

void EdSpecialObjectControl::AddMenuItem(eduimenu_s *, EdRef *, void *) {
}

void EdSpecialObjectControl::Process(EdInputContext &) {
}

void EdSpecialObjectControl::Render() {
}

void EdClassObjectNameControl::AddMenuItem(eduimenu_s *, EdRef *, void *) {
}

EdClassObjectNameControl::EdClassObjectNameControl() {
}

void EdClassObjectNameControl::Process(EdInputContext &) {
}

void EdClassObjectNameControl::Render() {
}

void EdClassObjectNameControl::cbButton(eduimenu_s *, eduiitem_s *, u32) {
}

void EdClassObjectNameControl::cbChanged(eduimenu_s *, eduiitem_s *, u32) {
}

void EdClassObjectNameControl::cbSelectClass(eduimenu_s *, eduiitem_s *, u32) {
}

void EdClassObjectNameControl::cbSelectObject(eduimenu_s *, eduiitem_s *, u32) {
}

void EdRef::CheckType(i32) {
}

EdRef::EdRef(char *, char *, i32, i32, i32, EdControl *, i32) {
}

void EdRef::GetAttributeData(void *, i32, i32, void *, i32) {
}

void EdRef::GetMemberData(void *, i32, void *, i32) {
}

void EdRef::GetMemberObject(void *) {
}

void EdRef::GetTypeSize(i32, i32) {
}

void EdRef::Serialise(EdStream &, i32 *) {
}

void EdRef::SetAttributeData(void *, i32, i32, void *, i32) {
}

void EdRef::SetMemberData(void *, i32, void *, i32, i16 *) {
}

void EdType::Serialise(EdStream &) {
}

EdStream::EdStream() {
}

EdStream::EdStream(MemoryBuffer *) {
}

EdStream::EdStream(MemoryBuffer *, MemoryBuffer *) {
}

void EdString::Set(char const *) {
}

EdString::~EdString() {
}

void EdSystem::Initalise(variptr_u &, variptr_u &, i32) {
}

void EdSystem::Process(float) {
}

void EdSystem::RegisterSubSystem(EdSubSystem *) {
}

void EdSystem::Render() {
}

void EdSystem::Reset() {
    for (EdSubSystem *subsystem = first_subsystem; subsystem != NULL; subsystem = subsystem->next) {
        subsystem->SubReset();
    }
}

__attribute__((weak)) void EdSubSystem::SubInitialise(variptr_u &, variptr_u &, i32) {
}

__attribute__((weak)) void EdSubSystem::SubReset() {
}

__attribute__((weak)) void EdSubSystem::SubProcess(float) {
}

__attribute__((weak)) void EdSubSystem::SubRender() {
}

void EdControl::AddMenuItem(eduimenu_s *, EdRef *, void *) {
}

void EdControl::Process(EdInputContext &) {
}

void EdControl::Render() {
}

void EdControl::SelectSubObject() {
}

void EdControl::Refresh() {
}

void EdControl::SetMenuItemAttr(i32, eduiitem_s *, eduiiattr_s *, eduiiattr_s *) {
}

void EdControl::cbSelected(eduimenu_s *, eduiitem_s *, u32) {
}

EdManMove::EdManMove() {
}

void EdManMove::Process(EdInputContext &, ClassObjectList &) {
}

void EdManMove::Render(ClassObjectList &) {
}

void EdRefKnot::GetMemberData(void *, i32, void *, i32) {
}

void EdRefKnot::SetMemberData(void *, i32, void *, i32, i16 *) {
}
