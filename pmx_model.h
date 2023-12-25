#ifndef __PMX_MODEL_H
#define __PMX_MODEL_H

#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

#define MAX(x, y) ((x) > (y) ? (x) : (y))
#define MIN(x, y) ((x) < (y) ? (x) : (y))

#ifdef NDEBUG
#define TRACE(...) fprintf(stderr, __VA_ARGS__)
#else
#define TRACE(...)
#endif

#define MAX_TEXT_LEN 64
#define ERROR_MSG_LEN 128

#define TOON_TEX 0
#define TOON_BUILTIN 1

#ifdef __cplusplus
extern "C" {
#endif // __cplusplus

typedef struct __attribute__((packed))
{
	char sig[4];
	float ver;
	uint8_t data_count;
	uint8_t text_enc;
	uint8_t uv_count;
	uint8_t vert_idx_size;
	uint8_t tex_idx_size;
	uint8_t mat_idx_size;
	uint8_t bone_idx_size;
	uint8_t morph_idx_size;
	uint8_t rb_idx_size;
} PMXHeader;

typedef struct 
{
	uint32_t len;
	char text[MAX_TEXT_LEN];
} PMXText;

typedef struct 
{
	PMXText name_jp;
	PMXText name_en;
	PMXText comm_jp;
	PMXText comm_en;
} PMXInfo;

typedef enum
{
	BDEF1 = 0, 
	BDEF2 = 1, 
	BDEF4 = 2,
	SDEF = 3
} PMXWeightType;

typedef struct 
{
	float pos[3];
	float normal[3];
	float uv[2];
	uint8_t weight_type;
	union {
		struct {
			uint32_t idx0;
		} bdef1;
		struct {
			uint32_t idx0;
			uint32_t idx1;
			float w0;
		} bdef2;
		struct {
			uint32_t idx[4];
			uint32_t w[4];
		} bdef4;
		struct {
			uint32_t idx0;
			uint32_t idx1;
			float w0;
			float c[3];
			float r0[3];
			float r1[3];
		} sdef;
	} weight;
	float edge_scale;
} PMXVert;

typedef struct 
{
	uint32_t indices[3];
} PMXFace;

typedef struct 
{
	PMXText name;
} PMXTex;

typedef struct
{
	PMXText name_jp;
	PMXText name_en;
	float diffuse[4];
	float specular[3];
	float power;
	float ambient[3];
	uint8_t draw_mode;
	float edge[4];
	float edge_size;
	uint32_t tex_idx;
	uint32_t env_idx;
	uint8_t env_mode;
	uint8_t toon_mode;
	uint32_t toon_idx;
	PMXText memo;
	uint32_t face_count;
} PMXMat;

typedef struct 
{
	uint32_t idx;
	uint8_t has_limit;
	struct {
		float lower[3];
		float upper[3];
	} limit;
} PMXIKLink;

#define BONE_FLAG_CONNECTED (1U << 0)
#define BONE_FLAG_ROTATABLE (1U << 1)
#define BONE_FLAG_MOVEABLE (1U << 2)
#define BONE_FLAG_DISPLAY (1U << 3)
#define BONE_FLAG_OPERATABLE (1U << 4)
#define BONE_FLAG_IK (1U << 5)
#define BONE_FLAG_LINK_DEFORM (1U << 7)
#define BONE_FLAG_LINK_ROTATION (1U << 8)
#define BONE_FLAG_LINK_MOVE (1U << 9)
#define BONE_FLAG_FIXED_AXIS (1U << 10)
#define BONE_FLAG_LOCAL_AXIS (1U << 11)
#define BONE_FLAG_PHYSICAL_TRANSFORM (1U << 12)
#define BONE_FLAG_EXT_PARENT_TRANSFORM (1U << 13)

typedef struct 
{
	PMXText name_jp;
	PMXText name_en;
	float pos[3];
	uint32_t parent;
	uint32_t transform_layer;
        uint16_t flags;
	union {
		uint32_t target;
		float offset[3];
	} tip;
	struct {
		uint32_t idx;
		float rate;
	} link;
	float fixed_axis[3];
	struct {
		float x[3];
		float z[3];
	} local_axis;
	uint32_t ext_parent_key;
	struct {
		uint32_t idx;
		uint32_t loop;
		float limit_angle;
		uint32_t link_count;
		PMXIKLink *links;
	} ik;
} PMXBone;

typedef enum
{
	MORPH_TYPE_GROUP = 0,
	MORPH_TYPE_VERTEX,
	MORPH_TYPE_BONE,
	MORPH_TYPE_UV,
	MORPH_TYPE_ADD_UV_1,
	MORPH_TYPE_ADD_UV_2,
	MORPH_TYPE_ADD_UV_3,
	MORPH_TYPE_ADD_UV_4,
	MORPH_TYPE_MATERIAL,
	MORPH_TYPE_FLIP,
	MORPH_TYPE_IMPULSE
} PMXMorphType;

typedef union {
	struct {
		uint32_t idx;
		float rate;
	} group_flip;
	struct {
		uint32_t idx;
		float offset[3];
	} vertex;
	struct {
		uint32_t idx;
		float move[3];
		float rotation[4];
	} bone;
	struct {
		uint32_t idx;
		float offset[4];
	} uv;
	struct {
		uint32_t idx;
		uint8_t method;
		float diffuse[4];
		float specular[3];
		float power;
		float ambient[3];
		float edge[4];
		float edge_size;
		float tex_tint[4];
		float env_tint[4];
		float toon_tint[4];
	} material;
	struct {
		uint32_t idx;
		uint8_t local;
		float velocity[3];
		float torque[3];
	} impulse;
} PMXMorphOffset; 

typedef struct 
{
	PMXText name_jp;
	PMXText name_en;
	uint8_t panel;
	uint8_t type;
	uint32_t offset_count;
	PMXMorphOffset *offsets;
} PMXMorph;

typedef enum
{
	FRAME_ELEM_TYPE_BONE = 0,
	FRAME_ELEM_TYPE_MORPH
} PMXFrameElemType;

typedef struct 
{
	uint8_t type;
	uint32_t idx;
} PMXFrameElement;

typedef struct 
{
	PMXText name_jp;
	PMXText name_en;
	uint8_t special;
	uint32_t elem_count;
	PMXFrameElement *elems;
} PMXFrame;

typedef enum
{
	RIGIDBODY_SHAPE_SPHERE = 0,
	RIGIDBODY_SHAPE_BOX,
	RIGIDBODY_SHAPE_CAPSULE
} PMXRigidBodyShape;

typedef enum
{
	RIGIDBODY_TYPE_STATIC = 0,
	RIGIDBODY_TYPE_DYNAMIC,
	RIGIDBODY_TYPE_COMBINE
} PMXRigidBodyType;

typedef struct
{
	PMXText name_jp;
	PMXText name_en;
	uint32_t bone_idx;
	uint8_t group;
	uint16_t no_collide_group;
	uint8_t shape;
	float shape_size[3];
	float pos[3];
	float rot[3];
	float mass;
	float move_decay;
	float rot_decay;
	float elastic;
	float friction;
	uint8_t type;
} PMXRigidBody;

typedef struct
{
	PMXText name_jp;
	PMXText name_en;
	uint8_t type;
	uint32_t idx1;
	uint32_t idx2;
	float pos[3];
	float rot[3];
	struct {
		float lower[3];
		float upper[3];
	} pos_limit;
	struct {
		float lower[3];
		float upper[3];
	} rot_limit;
	float spring_pos[3];
	float spring_rot[3];
} PMXJoint;

typedef struct 
{
	PMXHeader header;
	PMXInfo info;
	uint32_t vertex_count;
	PMXVert *vertices;
	uint32_t face_count;
	PMXFace *faces;
	uint32_t texture_count;
	PMXTex *textures;
	uint32_t material_count;
	PMXMat *materials;
	uint32_t bone_count;
	PMXBone *bones;
	uint32_t morph_count;
	PMXMorph *morphs;
	uint32_t frame_count;
	PMXFrame *frames;
	uint32_t rigidbody_count;
	PMXRigidBody *rigidbodies;
	uint32_t joint_count;
	PMXJoint *joints;
} PMXModel;

int pmx_parse(const char *src, PMXModel *dst);
void pmx_free(PMXModel *model);
const char *pmx_get_error_msg(void);

#ifdef __cplusplus
}
#endif // __cplusplus 

#endif // __PMX_MODEL_H 
