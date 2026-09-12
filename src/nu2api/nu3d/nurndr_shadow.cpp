#include "nu2api/nu3d/nucamera.h"
#include "nu2api/nu3d/nurndr.h"
#include "nu2api/numath/nuvec.h"

extern "C" {
    i32 NuRndrShadowCnt = 0;
    NURND_SHADOW_s NuRndrShadPolDat[128] = {};

    void NuRndrAddShadow(NUVEC *position, f32 radius, i32 opacity, i32 x_rotation, i32 y_rotation, i32 z_rotation) {
        i32 clipped = NuCameraClipTestPoints(position, 1, NULL);
        if (clipped == 0 && NuRndrShadowCnt <= 127) {
            NuRndrShadPolDat[NuRndrShadowCnt].position = *position;
            NuRndrShadPolDat[NuRndrShadowCnt].radius = radius;
            NuRndrShadPolDat[NuRndrShadowCnt].opacity = opacity;
            NuRndrShadPolDat[NuRndrShadowCnt].x_rotation = x_rotation;
            NuRndrShadPolDat[NuRndrShadowCnt].y_rotation = y_rotation;
            NuRndrShadPolDat[NuRndrShadowCnt].z_rotation = z_rotation;
            ++NuRndrShadowCnt;
        }
    }

    void NuRndrShadPolys(numtl_s *material) {
        static NURND_VERTEX3D vtx[4];
        if (NuRndrShadowCnt != 0) {
            for (i32 index = 0; index < NuRndrShadowCnt; ++index) {
                i32 i = index;
                NUVEC position = NuRndrShadPolDat[i].position;
                f32 radius = NuRndrShadPolDat[i].radius;
                i32 colour = 0xff0000ff;
                vtx[0].colour = colour;
                vtx[1].colour = colour;
                vtx[2].colour = colour;
                vtx[3].colour = colour;
                NUVEC a;
                a.x = -radius;
                a.y = 0.0f;
                a.z = radius;
                NUVEC b;
                b.x = radius;
                b.y = 0.0f;
                b.z = radius;
                if (NuRndrShadPolDat[i].y_rotation || NuRndrShadPolDat[i].z_rotation ||
                    NuRndrShadPolDat[i].x_rotation) {
                    NUMTX matrix __attribute__((aligned(16)));
                    NuMtxSetIdentity(&matrix);
                    if (NuRndrShadPolDat[i].y_rotation)
                        NuMtxRotateY(&matrix, (i16)NuRndrShadPolDat[i].y_rotation);
                    if (NuRndrShadPolDat[i].z_rotation)
                        NuMtxRotateZ(&matrix, (i16)NuRndrShadPolDat[i].z_rotation);
                    if (NuRndrShadPolDat[i].x_rotation)
                        NuMtxRotateX(&matrix, (i16)NuRndrShadPolDat[i].x_rotation);
                    NuVecMtxRotate(&a, &a, &matrix);
                    NuVecMtxRotate(&b, &b, &matrix);
                }
                vtx[0].position.x = position.x - b.x;
                vtx[0].position.y = position.y - b.y;
                vtx[0].position.z = position.z - b.z;
                vtx[0].u = 0.0f;
                vtx[0].v = 0.0f;
                vtx[1].position.x = position.x - a.x;
                vtx[1].position.y = position.y - a.y;
                vtx[1].position.z = position.z - a.z;
                vtx[1].u = 1.0f;
                vtx[1].v = 0.0f;
                vtx[2].position.x = position.x + a.x;
                vtx[2].position.y = position.y + a.y;
                vtx[2].position.z = position.z + a.z;
                vtx[2].u = 0.0f;
                vtx[2].v = 1.0f;
                vtx[3].position.x = position.x + b.x;
                vtx[3].position.y = position.y + b.y;
                vtx[3].position.z = position.z + b.z;
                vtx[3].u = 1.0f;
                vtx[3].v = 1.0f;
                NuRndrStrip3d(vtx, material, NULL, 4);
            }
            NuRndrShadowCnt = 0;
        }
    }
}
