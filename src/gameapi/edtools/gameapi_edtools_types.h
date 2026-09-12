#ifndef GAMEAPI_EDTOOLS_TYPES_H
#define GAMEAPI_EDTOOLS_TYPES_H
#pragma once

#include "nu2api/nucore/fixed_width.h"
#include "nu2api/nucore/nuvuvec.hpp"
#include "nu2api/numath/nuvec.h"

struct ClassObjectList;
struct EdBitControl;
struct EdClass;
struct EdClassInterface;
struct EdClassObjectNameControl;
struct EdColourControl;
struct EdControl;
struct EdDefunctList;
struct EdEnumControl;
struct EdFileInputStream;
struct EdFileOutputStream;
struct EdInputContext;
struct EdInputStream;
struct EdManMove;
struct EdManRotate;
struct EdManScale;
struct EdManipulator;
struct EdMatrixControl;
struct EdMember;
struct EdObjectNotifier;
struct EdOutputStream;
struct EdRef;
struct EdRefKnot;
struct EdRefPlaceable;
struct EdRefSpecialObject;
struct EdRefSpline;
struct EdRegistry;
struct EdSfxNameControl;
struct EdSpecialObjectControl;
struct EdStream;
struct EdString;
struct EdStringControl;
struct EdSubSystem;
struct EdSystem;
struct EdType;
struct EdVectorControl;
struct EditorSettings;
struct KnotHelper;
struct MemoryBuffer;
struct SplineHelper;
struct SplineKnot;
struct SplineKnotList;
struct SplineObject;
struct SplinePointBlock;
struct SplinePointList;
struct SplineTool;
struct VuMtx;
struct VuVec;
struct burnset_s;
struct eduiiattr_s;
struct eduiitem_s;
struct eduimenu_s;
struct nucamera_s;
struct nugscn_s;
struct nugspline_s;
struct nupad_s;
struct nuvec_s;
struct part_typedesc_s;
union variptr_u;

struct ClassObjectList;
struct EdMember {
    struct VTable {
        void *(*get_member_object)(EdMember *, void const *);
        void (*get_member_data)(EdMember *, void const *, i32, void *, i32);
    };

    VTable *vtable;
    EdMember *next;
    u32 reserved_08;
    i32 type_id;
    u8 reserved_10[8];
    i32 array_size;
    i32 class_marker;
    u32 reserved_20;
    u16 replication_group;
    u16 reserved_26;
};
struct EdObjectNotifier {};
struct EdSubSystem {
    virtual ~EdSubSystem();
    virtual void SubInitialise(variptr_u &, variptr_u &, i32);
    virtual void SubReset();
    virtual void SubProcess(float);
    virtual void SubRender();

    EdSubSystem *next;
    EdSubSystem *previous;
};
struct MemoryBuffer;
struct VuMtx;
struct VuVec;
struct burnout_s {
    i32 active;
    NUVEC position;
    float field_10, field_14, field_18, field_1c, field_20;
};
struct burn_parameters_s {
    i32 field_00;
    float field_04, field_08, field_0c, field_10, field_14;
    i32 field_18;
    float field_1c, field_20, field_24;
    i32 field_28;
    float field_2c, field_30, field_34, field_38, field_3c;
    float field_40, field_44, field_48, field_4c, field_50;
};
struct burnset_s {
    burn_parameters_s parameters;
    burn_parameters_s parameters_copy;
    i32 field_a8, field_ac, field_b0, field_b4;
    float field_b8, field_bc, field_c0, field_c4, field_c8, field_cc;
    burnout_s burnouts[32];
    i32 active_count;
    i32 selected_index;
    float field_558, field_55c;
    i32 field_560;
};
struct eduiiattr_s {};
struct eduiitem_s;
struct eduimenu_s;
struct nucamera_s;
struct nugscn_s;
struct nugspline_s;
struct nupad_s;
struct nuvec_s;
union variptr_u;

struct edanim_param_s {
    i32 instance_id;
    i32 effect_count;
    i32 sound_count;
    i32 field_00c;
    i32 field_010;
    float field_014;
    float field_018;
    char effect_names[8][16];
    i32 effect_ids[8];
    i32 effect_intervals[8];
    i32 effect_flags[8];
    float effect_positions[8][3];
    i16 effect_angles[8];
    i16 effect_angle_ranges[8];
    float field_17c;
    char sound_names[8][0x10];
    i32 sound_ids[8];
    i32 sound_flags[8];
    float sound_values[8];
    float sound_positions[8][3];
    i32 platform_id;
    float bounce_impulse;
    float bounce_spring;
    float bounce_damping;
    i8 page;
    u8 reserved_2d1[3];
};
static_assert(sizeof(edanim_param_s) == 0x2d4, "edanim_param_s size");

struct EdBitControl {
    void AddMenuItem(eduimenu_s *, EdRef *, void *);
    void Refresh();
    void cbButton(eduimenu_s *, eduiitem_s *, u32);
    void cbChanged(eduimenu_s *, eduiitem_s *, u32);
    void cbSelectItem(eduimenu_s *, eduiitem_s *, u32);
};
struct EdClass {
    u8 reserved_00[8];
    EdMember *members;
    u8 reserved_0c[0xc];

    void AddType(EdRef *);
    void CopyObject(void *, void *);
    void FindMember(EdMember *, void *, i32, i32);
    void FindObject(char *);
    void FindTypeRef(char *, i32);
    void FindTypeRef(i32, i32);
    void GetStreamClasses(EdStream &, i32 *, i32 &, i32);
    void Serialise(EdStream &, i32 *);
    void SerialiseObject(EdStream &, void *);
    void SerialiseObject(EdStream &, void *, EdClass *, EdRegistry *);
    void SerialiseObjectHeader(EdStream &, void *);
};
struct EdClassInterface {
    void DistanceToObject(VuVec &, VuVec &, void *, EdRef **);
    void DistanceToObject(VuVec &, void *, EdRef **);
    void GetNextObject(void *, i32 (*)(void *));
};
struct EdClassObjectNameControl {
    void AddMenuItem(eduimenu_s *, EdRef *, void *);
    EdClassObjectNameControl();
    void Process(EdInputContext &);
    void Render();
    void cbButton(eduimenu_s *, eduiitem_s *, u32);
    void cbChanged(eduimenu_s *, eduiitem_s *, u32);
    void cbSelectClass(eduimenu_s *, eduiitem_s *, u32);
    void cbSelectObject(eduimenu_s *, eduiitem_s *, u32);
};
struct EdColourControl {
    void AddMenuItem(eduimenu_s *, EdRef *, void *);
    EdColourControl();
    void Refresh();
    void cbButton(eduimenu_s *, eduiitem_s *, u32);
    void cbChanged(eduimenu_s *, eduiitem_s *, u32);
    void cbColourSelected(eduimenu_s *, eduiitem_s *, u32);
};
struct EdControl {
    void AddMenuItem(eduimenu_s *, EdRef *, void *);
    void Process(EdInputContext &);
    void Refresh();
    void Render();
    void SelectSubObject();
    void SetMenuItemAttr(i32, eduiitem_s *, eduiiattr_s *, eduiiattr_s *);
    void cbSelected(eduimenu_s *, eduiitem_s *, u32);
};
struct EdDefunctList {
    void ReviveAll(i32);
};
struct EdEnumControl {
    void AddMenuItem(eduimenu_s *, EdRef *, void *);
    void GetEnumString(i32);
    void GetEnumValue(char *);
    void Refresh();
    void cbButton(eduimenu_s *, eduiitem_s *, u32);
    void cbChanged(eduimenu_s *, eduiitem_s *, u32);
    void cbSelectItem(eduimenu_s *, eduiitem_s *, u32);
};
struct EdFileInputStream {
    void BeginBlock(char const *);
    void Eat(i32, i32);
    void EndBlock();
    void Open(i32, i32);
    void SerialiseBuffer(void *, i32, i32);
};
struct EdFileOutputStream {
    void BeginBlock(char const *);
    void Eat(i32, i32);
    void EndBlock();
    void Open(i32, i32);
    void SerialiseBuffer(void *, i32, i32);
};
struct EdInputContext {
    u8 reserved_00[0x48];
    f32 current_time;
    f32 repeat_window;
    u8 held[40];
    u8 pressed[40];
    u8 released[40];
    u8 repeated[40];
    u8 cleared[40];
    f32 values[40];
    f32 repeat_times[40];

    void Clear(i32);
    EdInputContext();
    f32 Get(i32);
    f32 GetHold(i32);
    f32 GetPress(i32);
    f32 GetRelease(i32);
    f32 GetRepeat(i32);
    void Set(i32, float, float);
    void Update(nucamera_s *, nupad_s *, float, bool);
};
struct EdInputStream {
    void SerialiseString(char **);
    void SerialiseString(char **, i32);
    void SerialiseString(char *, i32);
};
struct EdManMove {
    EdManMove();
    void Process(EdInputContext &, ClassObjectList &);
    void Render(ClassObjectList &);
};
struct EdManRotate {
    EdManRotate();
    void Process(EdInputContext &, ClassObjectList &);
    void Render(ClassObjectList &);
    void RotateItem(EdInputContext &, ClassObjectList &, i32, i32);
};
struct EdManScale {
    EdManScale();
    void Process(EdInputContext &, ClassObjectList &);
    void Render(ClassObjectList &);
};
struct EdManipulator {
    void DrawAxis(VuVec &, VuMtx *);
    void DrawRotator(VuVec &);
    void GetAxisLocators(VuVec &, VuVec *, VuMtx *);
    void Process(EdInputContext &, ClassObjectList &);
    void Render(ClassObjectList &);
    void SelectAxis(EdInputContext &, VuVec &, VuVec &, VuVec &, VuMtx *);
    void SelectRotator(EdInputContext &, VuVec &, VuVec &);
};
struct EdMatrixControl {
    void AddMenuItem(eduimenu_s *, EdRef *, void *);
    void Destroy();
    EdMatrixControl();
    void Refresh();
    void SetMenuItemAttr(i32, eduiitem_s *, eduiiattr_s *, eduiiattr_s *);
    void cbButton(eduimenu_s *, eduiitem_s *, u32);
    void cbChanged(eduimenu_s *, eduiitem_s *, u32);
    void cbSelected(eduimenu_s *, eduiitem_s *, u32);
};
struct EdOutputStream {
    void SerialiseString(char **);
    void SerialiseString(char **, i32);
    void SerialiseString(char *, i32);
};
struct EdRef {
    void CheckType(i32);
    EdRef(char *, char *, i32, i32, i32, EdControl *, i32);
    void GetAttributeData(void *, i32, i32, void *, i32);
    void GetMemberData(void *, i32, void *, i32);
    void GetMemberObject(void *);
    void GetTypeSize(i32, i32);
    void Serialise(EdStream &, i32 *);
    void SetAttributeData(void *, i32, i32, void *, i32);
    void SetMemberData(void *, i32, void *, i32, i16 *);
};
struct EdRefKnot {
    void GetMemberData(void *, i32, void *, i32);
    void SetMemberData(void *, i32, void *, i32, i16 *);
};
struct EdRefPlaceable {
    void GetMemberData(void *, i32, void *, i32);
    void SetMemberData(void *, i32, void *, i32, i16 *);
};
struct EdRefSpecialObject {
    void GetMemberData(void *, i32, void *, i32);
    void SetMemberData(void *, i32, void *, i32, i16 *);
};
struct EdRefSpline {
    void GetMemberData(void *, i32, void *, i32);
    void SetMemberData(void *, i32, void *, i32, i16 *);
};
struct EdRegistry {
    u32 reserved_00;
    EdType *types;
    EdClass *classes;
    u8 reserved_0c[0x10];
    i32 type_count;
    u32 reserved_20;
    i32 class_count;
    u32 reserved_28;
    i32 object_count;

    void AddMapping(char *, char *);
    void AddObjectNotifier(EdObjectNotifier *);
    void ClassIFaceProcess(EdClass *, void *, EdInputContext &);
    void ClassIFaceProcess(i32, void *, EdInputContext &);
    void ClassIFaceRender(EdClass *, void *, i32);
    void ClassIFaceRender(i32, void *, i32);
    void CreateObject(EdClassInterface *, void *, i32, i32, i32);
    void DefunctObject(EdClassInterface *, void *, i32, i32);
    void DestroyObject(EdClassInterface *, void *, i32, i32);
    void Flush();
    void GetClass(char *);
    EdClass *GetClass(i32);
    i32 GetClassId(EdClass *);
    void GetClassId(char *);
    void GetStreamClassMapping(EdStream &, i32 *, i32 &, i32);
    void GetType(char *);
    EdType *GetType(i32);
    void GetTypeId(char *);
    void Initialise(variptr_u &, variptr_u &, i32, i32, i32, i32);
    void MapName(char *);
    void NotifyCreateObject(void *, EdClass *, void *, i32, i32, i32);
    void NotifyDefunctObject(void *, EdClass *, i32);
    void NotifyDestroyObject(void *, EdClass *, i32, i32);
    void NotifyReviveObject(void *, EdClass *, i32);
    void RegisterBaseTypes();
    void RegisterClass(char *, EdClassInterface *, i32);
    void RegisterType(char *, i32, void (*)(EdStream &, void *, i32));
    void Serialise(EdStream &);
    void SerialiseObjects(EdStream &, EdRegistry *);
};
struct EdSfxNameControl {
    void AddMenuItem(eduimenu_s *, EdRef *, void *);
    EdSfxNameControl();
    void cbButton(eduimenu_s *, eduiitem_s *, u32);
    void cbChanged(eduimenu_s *, eduiitem_s *, u32);
    void cbSelectSfx(eduimenu_s *, eduiitem_s *, u32);
};
struct EdSpecialObjectControl {
    EdSpecialObjectControl();
    void AddMenuItem(eduimenu_s *, EdRef *, void *);
    void Process(EdInputContext &);
    void Render();
    void cbButton(eduimenu_s *, eduiitem_s *, u32);
    void cbChanged(eduimenu_s *, eduiitem_s *, u32);
    void cbSelectObject(eduimenu_s *, eduiitem_s *, u32);
};
struct EdStream {
    EdStream();
    EdStream(MemoryBuffer *);
    EdStream(MemoryBuffer *, MemoryBuffer *);
};
struct EdString {
    void Set(char const *);
    ~EdString();
};
struct EdStringControl {
    void AddMenuItem(eduimenu_s *, EdRef *, void *);
    EdStringControl();
    void GetVal(char *, i32);
    void Refresh();
    void SetVal(char const *);
    void cbChanged(eduimenu_s *, eduiitem_s *, u32);
    void cbPress(eduimenu_s *, eduiitem_s *, u32);
};
struct EdSystem {
    EdSubSystem *first_subsystem;
    EdSubSystem *last_subsystem;
    i32 subsystem_count;

    void Initalise(variptr_u &, variptr_u &, i32);
    void Process(float);
    void RegisterSubSystem(EdSubSystem *);
    void Render();
    void Reset();
};
struct EdType {
    u32 reserved_00;
    i32 size;
    u32 reserved_08;

    void Serialise(EdStream &);
};

static_assert(sizeof(void *) != 4 || sizeof(EdClass) == 0x18, "EdClass 32-bit size");
static_assert(sizeof(void *) != 4 || sizeof(EdType) == 0xc, "EdType 32-bit size");
static_assert(sizeof(void *) != 4 || sizeof(EdMember) == 0x28, "EdMember 32-bit size");
static_assert(sizeof(void *) != 4 || offsetof(EdRegistry, types) == 0x4, "EdRegistry::types 32-bit offset");
static_assert(sizeof(void *) != 4 || offsetof(EdRegistry, classes) == 0x8, "EdRegistry::classes 32-bit offset");
static_assert(sizeof(void *) != 4 || offsetof(EdRegistry, type_count) == 0x1c, "EdRegistry::type_count 32-bit offset");
static_assert(sizeof(void *) != 4 || offsetof(EdRegistry, class_count) == 0x24,
              "EdRegistry::class_count 32-bit offset");
static_assert(sizeof(void *) != 4 || offsetof(EdRegistry, object_count) == 0x2c,
              "EdRegistry::object_count 32-bit offset");
struct EdVectorControl {
    void AddMenuItem(eduimenu_s *, EdRef *, void *);
    void Destroy();
    EdVectorControl();
    void Refresh();
    void cbButton(eduimenu_s *, eduiitem_s *, u32);
    void cbChanged(eduimenu_s *, eduiitem_s *, u32);
    void cbSelected(eduimenu_s *, eduiitem_s *, u32);
};
struct EditorSettings {
    void AddMenuItems(eduimenu_s *);
    EditorSettings();
    void Serialise(EdStream &);
};
struct KnotHelper {
    void CreateObject(void *, i32, i32);
    void DestroyObject(void *, i32);
    void DistanceToObject(VuVec &, VuVec &, void *, EdRef **);
    void GetNextObject(void *);
    void GetNumObjects();
    void Process(void *, EdInputContext &);
    void Render(void *, i32);
};
struct SplineHelper {
    u8 reserved_0x00[8];
    SplineObject *first_object;
    u8 reserved_0x0c[4];
    i32 object_count;

    void AddMenuItems(eduimenu_s *);
    void ClearLevel(i32);
    void CreateObject(void *, i32, i32);
    void DestroyObject(void *, i32);
    void Find(char *);
    void Find(char *, SplineObject **, i32);
    void *GetNextObject(void *);
    i32 GetNumObjects();
    void Initialise();
    void PostLoadInitialisation(MemoryBuffer *, MemoryBuffer *);
    void PreLoadInitialisation(MemoryBuffer *, MemoryBuffer *);
    void Process(void *, EdInputContext &);
    void Render(void *, i32);
    void SerialiseObject(EdStream &, void *);
    void cbEdSplineAutoGenPoints(eduimenu_s *, eduiitem_s *, u32);
    void cbEdSplineReGenPoints(eduimenu_s *, eduiitem_s *, u32);
    void cbEdSplineReverseSpline(eduimenu_s *, eduiitem_s *, u32);
    void cbEdSplineSmoothKnot(eduimenu_s *, eduiitem_s *, u32);
    void cbEdSplineSmoothSpline(eduimenu_s *, eduiitem_s *, u32);
};
struct SplineKnot {
    SplineKnot *next;
    u8 reserved_0x04[4];
    VuVec position;

    void Smooth();
};
struct SplineKnotList {
    SplineKnot *first;

    i32 GetPoint(i32, VuVec &);
};
struct SplineObject {
    u8 reserved_0x00[4];
    SplineObject *next;

    void Clone();
    void Draw(i32, i32, i32, float);
    void DropPoint(VuVec &);
    void GenBezierPoints();
    void GenLinearPoints();
    void GenPoints();
    void ReverseKnots();
    void SmoothKnots();
};
struct SplinePointBlock {
    SplinePointBlock *next;
    u8 reserved_0x08[8];
    i32 point_count;
    VuVec *points;

    void Draw();
    SplinePointBlock();
    SplinePointBlock(i32);
    virtual ~SplinePointBlock();
};
struct SplinePointList {
    SplinePointBlock *first;

    void AddPoint(VuVec &);
    void Clear();
    void Draw();
    i32 GetNumPoints();
    i32 GetPoint(i32, VuVec &);
};
struct SplineTool {
    void Initialise(variptr_u &, variptr_u &, i32);
    void Process(EdInputContext &);
    void Render();
};

#endif // GAMEAPI_EDTOOLS_TYPES_H
