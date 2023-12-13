#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <float.h>
#include <assert.h>

#define NDEBUG
#ifdef NDEBUG
#define DBG_LOG(x, ...) fprintf(stderr, (x), __VA_ARGS__)
#else
#define DBG_LOG(x, ...)
#endif

#define MAX(x, y) ((x) > (y) ? (x) : (y))

#define MAX_TEXT_LEN 64
#define ERROR_MSG_LEN 128

#define TOON_TEX 0
#define TOON_BUILTIN 1

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
	uint32_t indicies[3];
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

typedef struct 
{
	PMXHeader header;
	PMXInfo info;
} PMXModel;

static PMXHeader header;
static PMXModel parsed;
static PMXInfo info;
static char *model;
static char error_msg[ERROR_MSG_LEN];

const char *get_field(const char *src, void *dst, size_t size, size_t count);

const char *get_text(const char *src, PMXText *dst)
{
	uint32_t len;
	src = get_field(src, &len, sizeof(len), 1);
	dst->len = len;
	strncpy(dst->text, src, MAX(sizeof(dst->text), len));
	src += len;
	return src;
}

const char *get_field(const char *src, void *dst, size_t size, size_t count)
{
	for (size_t i = 0; i < count; ++i) {
		switch (size) {
			case 1:
				*(uint8_t *)dst = *(uint8_t *)src;
				src += sizeof(uint8_t);
				break;
			case 2: 
				*(uint16_t *)dst = *(uint16_t *)src;
				src += sizeof(uint16_t);
				break;
			case 4: 
				*(uint32_t *)dst = *(uint32_t *)src;
				src += sizeof(uint32_t);
				break;
			defalut:	
				return NULL;
		}
	}
	
	return src;
}

const char *pmx_parse_info(const char *src, PMXInfo *dst)
{
	src = get_text(src, &info.name_jp);
       	src = get_text(src, &info.name_en);
	src = get_text(src, &info.comm_jp);
	src = get_text(src, &info.comm_en);	
	return src;
}

const char *pmx_parse_vert(const char *src, PMXVert *dst, size_t count)
{
	for (size_t i = 0; i < count; ++i) {
		src = get_field(src, &dst[i].pos, sizeof(float), 3);
	       	src = get_field(src, &dst[i].normal, sizeof(float), 3);	
		src = get_field(src, &dst[i].uv, sizeof(float), 2);
		src = get_field(src, &dst[i].weight_type, sizeof(uint8_t), 1);

		size_t idx_size = header.bone_idx_size;
		switch (dst[i].weight_type) {
			case BDEF1:
				src = get_field(src, &dst[i].weight.bdef1.idx0, idx_size, 1); 
				break;
			case BDEF2:
				src = get_field(src, &dst[i].weight.bdef2.idx0, idx_size, 2);
				src = get_field(src, &dst[i].weight.bdef2.w0, sizeof(float), 1); 
				break;
			case BDEF4:
				src = get_field(src, dst[i].weight.bdef4.idx, idx_size, 4);
				src = get_field(src, dst[i].weight.bdef4.w, sizeof(float), 4);
				break;
			case SDEF:
				src = get_field(src, &dst[i].weight.sdef.idx0, idx_size, 2);
				src = get_field(src, &dst[i].weight.sdef.w0, sizeof(float), 1);
				src = get_field(src, &dst[i].weight.sdef.c, sizeof(float), 9);
				break;
			default:
				return NULL;
		}
		src = get_field(src, &dst[i].edge_scale, sizeof(float), 1);	
		// DBG_LOG("Vertex %zu: (%.2f, %.2f, %.2f)\n", i, dst[i].pos[0], dst[i].pos[1], dst[i].pos[2]);
	}

	return src;
}

const char *pmx_parse_face(const char *src, PMXFace *dst, size_t count)
{
	src = get_field(src, dst, header.vert_idx_size, 3 * count);
	return src;
}

const char *pmx_parse_tex(const char* src, PMXTex *dst, size_t count)
{
	for (size_t i = 0; i < count; ++i) 
		src = get_text(src, &dst[i].name);
	return src;
}

const char *pmx_parse_mat(const char *src, PMXMat *dst, size_t count)
{
	for (size_t i = 0; i < count; ++i) {
		src = get_text(src, &dst[i].name_jp);
		src = get_text(src, &dst[i].name_en);
		src = get_field(src, dst[i].diffuse, sizeof(float), 4);
		src = get_field(src, dst[i].specular, sizeof(float), 3);
		src = get_field(src, &dst[i].power, sizeof(float), 1);
		src = get_field(src, dst[i].ambient, sizeof(float), 3);
		src = get_field(src, &dst[i].draw_mode, sizeof(uint8_t), 1);
		src = get_field(src, dst[i].edge, sizeof(float), 4);
		src = get_field(src, &dst[i].edge_size, sizeof(float), 1);
		src = get_field(src, &dst[i].tex_idx, header.tex_idx_size, 1);
		src = get_field(src, &dst[i].env_idx, header.tex_idx_size, 1);
		src = get_field(src, &dst[i].env_mode, sizeof(uint8_t), 1);
	        src = get_field(src, &dst[i].toon_mode, sizeof(uint8_t), 1);
		src = get_field(src, &dst[i].toon_idx, 
			dst[i].toon_mode == TOON_TEX ? header.tex_idx_size : 1, 1);
		src = get_text(src, &dst[i].memo);
		src = get_field(src, &dst[i].face_count, sizeof(uint32_t), 1);
	}

	return src;
}

const char *pmx_parse_ik(const char *src, PMXIKLink *dst, size_t count);

const char *pmx_parse_bone(const char *src, PMXBone *dst, size_t count)
{
	for (size_t i = 0; i < count; ++i) {
		src = get_text(src, &dst[i].name_jp);
		src = get_text(src, &dst[i].name_en);
		src = get_field(src, dst[i].pos, sizeof(float), 3);
		src = get_field(src, &dst[i].parent, header.bone_idx_size, 1);
		src = get_field(src, &dst[i].transform_layer, sizeof(uint32_t), 1);
		src = get_field(src, &dst[i].flags, sizeof(uint16_t), 1);

		uint16_t flags = dst[i].flags;
		if (flags & BONE_FLAG_CONNECTED) 
			src = get_field(src, &dst[i].tip.target, header.bone_idx_size, 1);
		else
			src = get_field(src, &dst[i].tip.offset, sizeof(float), 3);
		
		if (flags & BONE_FLAG_LINK_ROTATION || flags & BONE_FLAG_LINK_MOVE) {
			src = get_field(src, &dst[i].link.idx, header.bone_idx_size, 1);
			src = get_field(src, &dst[i].link.rate, sizeof(float), 1);
		}

		if (flags & BONE_FLAG_FIXED_AXIS)
			src = get_field(src, &dst[i].fixed_axis, sizeof(float), 3);

		if (flags & BONE_FLAG_LOCAL_AXIS)
			src = get_field(src, &dst[i].local_axis, sizeof(float), 3);

		if (flags & BONE_FLAG_EXT_PARENT_TRANSFORM)
			src = get_field(src, &dst[i].ext_parent_key, sizeof(uint32_t), 1);

		if (flags & BONE_FLAG_IK) {
			src = get_field(src, &dst[i].ik.idx, header.bone_idx_size, 1);
			src = get_field(src, &dst[i].ik.loop, sizeof(uint32_t), 1);
			src = get_field(src, &dst[i].ik.limit_angle, sizeof(float), 1);
			src = get_field(src, &dst[i].ik.link_count, sizeof(uint32_t), 1);
			
			size_t link_count = dst[i].ik.link_count;
			if (link_count > 0) {
				dst[i].ik.links = malloc(link_count * sizeof(PMXIKLink));
				src = pmx_parse_ik(src, dst[i].ik.links, link_count);
			}
		}	
	}

	return src;
}

const char *pmx_parse_ik(const char *src, PMXIKLink *dst, size_t count)
{
	for (size_t i = 0; i < count; ++i) {
		src = get_field(src, &dst[i].idx, header.bone_idx_size, 1);
		src = get_field(src, &dst[i].has_limit, sizeof(uint8_t), 1);
		if (dst[i].has_limit) {
			src = get_field(src, &dst[i].limit.lower, sizeof(float), 3);
			src = get_field(src, &dst[i].limit.upper, sizeof(float), 3);
		}
	}

	return src;
}

uint8_t pmx_parse(const char *src, PMXModel *dst)
{
	if (strncmp(src, "PMX ", 4)) {
		snprintf(error_msg, ERROR_MSG_LEN, "Not a pmx file\n");
		return 1;
	}
	
	memcpy(&header, src, sizeof(header));
	if (header.uv_count > 0) {
		snprintf(error_msg, ERROR_MSG_LEN, "Additional UV is unsupported\n");
		return 1;
	}
	src += sizeof(header);

	src = pmx_parse_info(src, &info);
	
	uint32_t vert_count;
	src = get_field(src, &vert_count, sizeof(vert_count), 1);
	DBG_LOG("Vertex count: %u\n", vert_count);
	PMXVert *verts = calloc(vert_count, sizeof(PMXVert));
	src = pmx_parse_vert(src, verts, vert_count);
	if (!src) {
		snprintf(error_msg, ERROR_MSG_LEN, "Failed to parse verticies\n");
		return 1;
	}

	uint32_t face_count;
	src = get_field(src, &face_count, sizeof(face_count), 1);
	face_count /= 3;
	DBG_LOG("Face count: %u\n", face_count);
	PMXFace *faces = calloc(face_count, sizeof(PMXFace));
	src = pmx_parse_face(src, faces, face_count);
	if (!src) {
		snprintf(error_msg, ERROR_MSG_LEN, "Failed to parse faces\n");
		return 1;
	}

	uint32_t tex_count;
	src = get_field(src, &tex_count, sizeof(tex_count), 1);
	DBG_LOG("Texture count: %u\n", tex_count);
	PMXTex *texs = calloc(tex_count, sizeof(PMXTex));
	src = pmx_parse_tex(src, texs, tex_count);
	assert(src);

	uint32_t mat_count;
	src = get_field(src, &mat_count, sizeof(mat_count), 1);
	DBG_LOG("Material count: %u\n", mat_count);
	PMXMat *mats = calloc(mat_count, sizeof(PMXMat));
	src = pmx_parse_mat(src, mats, mat_count);
	assert(mat);

	free(texs);	
	free(faces);
	free(verts);
	return 0;
}

int main(void)
{
	const char *filepath = u8"model/tsumi_miku_v2.1/000 ミクさん.pmx";
	
	FILE *fp = fopen(filepath, "rb");
	if(!fp) {
		fprintf(stderr, "ERROR: Could not open %s\n", filepath);
		exit(EXIT_FAILURE);
	}
	
	fseek(fp, 0, SEEK_END);
	size_t filesize = ftell(fp);
	fseek(fp, 0, SEEK_SET);
	DBG_LOG("Size: %zu\n", filesize);

	model = (char *)malloc(filesize);
	if (!model) {
		fprintf(stderr, "ERROR: Could not allocate memory\n");
		exit(EXIT_FAILURE);
	}

	if (fread(model, sizeof(char), filesize, fp) != filesize) {
		fprintf(stderr, "ERROR: Could not read %s\n", filepath);
		exit(EXIT_FAILURE);
	}
	
	if (pmx_parse(model, &parsed)) {
		fprintf(stderr, "ERROR: Failed to parse model.\nMessage: %s", error_msg);
		exit(EXIT_FAILURE);
	}

	free(model);

	return 0;
}

