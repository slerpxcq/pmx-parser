#include "pmx_model.h"

static char error_msg[ERROR_MSG_LEN];

static const char *get_field(const char *src, void *dst, size_t src_size, size_t count)
{
	for (size_t i = 0; i < count; ++i) {
		switch (src_size) {
		case 1:
			*(uint8_t *)dst = *(uint8_t *)src;
			src += 1;
			dst += 1;
			break;
		case 2: 
			*(uint16_t *)dst = *(uint16_t *)src;
			src += 2;
			dst += 2;
			break;
		case 4:
			*(uint32_t *)dst = *(uint32_t *)src;
			src += 4;
			dst += 4;
			break;
		}
	}
				
	return src;
}

static const char *get_field2(const char *src, void *dst, size_t src_size, size_t dst_size, size_t count) 
{
	for (size_t i = 0; i < count; ++i) {
		switch (src_size) {
		case 1:
			*(uint8_t *)dst = *(uint8_t *)src;
			src += 1;
			break;
		case 2: 
			*(uint16_t *)dst = *(uint16_t *)src;
			src += 2;
			break;
		case 4:
			*(uint32_t *)dst = *(uint32_t *)src;
			src += 4;
			break;
		}
		dst += dst_size;
	}

	return src;
}

static const char *get_text(const char *src, PMXText *dst)
{
	uint32_t len;

	src = get_field(src, &len, sizeof(uint32_t), 1);
	dst->len = len;
	memcpy(dst->text, src, MIN(len, MAX_TEXT_LEN));
	src += len;
	return src;
}

static const char *pmx_parse_info(const char *src, PMXInfo *dst)
{
	src = get_text(src, &dst->name_jp);
       	src = get_text(src, &dst->name_en);
	src = get_text(src, &dst->comm_jp);
	src = get_text(src, &dst->comm_en);
	return src;
}

static const char *pmx_parse_vert(const char *src, const PMXHeader *header, PMXVert *dst, size_t count)
{
	for (size_t i = 0; i < count; ++i) {
		src = get_field(src, &dst[i].pos, sizeof(float), 3);
	       	src = get_field(src, &dst[i].normal, sizeof(float), 3);	
		src = get_field(src, &dst[i].uv, sizeof(float), 2);
		src = get_field(src, &dst[i].weight_type, sizeof(uint8_t), 1);

		size_t idx_size = header->bone_idx_size;
		switch (dst[i].weight_type) {
			case BDEF1:
				src = get_field2(src, &dst[i].weight.bdef1.idx0, idx_size, sizeof(uint32_t), 1); 
				break;
			case BDEF2:
				src = get_field2(src, &dst[i].weight.bdef2.idx0, idx_size, sizeof(uint32_t), 2);
				src = get_field(src, &dst[i].weight.bdef2.w0, sizeof(float), 1); 
				break;
			case BDEF4:
				src = get_field2(src, dst[i].weight.bdef4.idx, idx_size, sizeof(uint32_t), 4);
				src = get_field(src, dst[i].weight.bdef4.w, sizeof(float), 4);
				break;
			case SDEF:
				src = get_field2(src, &dst[i].weight.sdef.idx0, idx_size, sizeof(uint32_t), 2);
				src = get_field(src, &dst[i].weight.sdef.w0, sizeof(float), 1);
				src = get_field(src, &dst[i].weight.sdef.c, sizeof(float), 9);
				break;
			default:
				return NULL;
		}
		src = get_field(src, &dst[i].edge_scale, sizeof(float), 1);	
	}

	return src;
}

static const char *pmx_parse_face(const char *src, const PMXHeader *header, PMXFace *dst, size_t count)
{
	for (size_t i = 0; i < count; ++i)
		src = get_field2(src, &dst[i].indices, header->vert_idx_size, sizeof(uint32_t), 3);
	return src;
}

static const char *pmx_parse_tex(const char* src, PMXTex *dst, size_t count)
{
	for (size_t i = 0; i < count; ++i) 
		src = get_text(src, &dst[i].name);
	return src;
}

static const char *pmx_parse_mat(const char *src, const PMXHeader *header, PMXMat *dst, size_t count)
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
		src = get_field2(src, &dst[i].tex_idx, header->tex_idx_size, sizeof(uint32_t), 1);
		src = get_field2(src, &dst[i].env_idx, header->tex_idx_size, sizeof(uint32_t), 1);
		src = get_field(src, &dst[i].env_mode, sizeof(uint8_t), 1);
	        src = get_field(src, &dst[i].toon_mode, sizeof(uint8_t), 1);
		src = get_field2(src, &dst[i].toon_idx, 
			dst[i].toon_mode == TOON_TEX ? header->tex_idx_size : 1, sizeof(uint32_t), 1);
		src = get_text(src, &dst[i].memo);
		src = get_field(src, &dst[i].face_count, sizeof(uint32_t), 1);
	}

	return src;
}

static const char *pmx_parse_ik(const char *src, const PMXHeader *header, PMXIKLink *dst, size_t count)
{
	for (size_t i = 0; i < count; ++i) {
		src = get_field2(src, &dst[i].idx, header->bone_idx_size, sizeof(uint32_t), 1);
		src = get_field(src, &dst[i].has_limit, sizeof(uint8_t), 1);
		if (dst[i].has_limit) {
			src = get_field(src, &dst[i].limit.lower, sizeof(float), 3);
			src = get_field(src, &dst[i].limit.upper, sizeof(float), 3);
		}
	}

	return src;
}

static const char *pmx_parse_bone(const char *src, const PMXHeader *header, PMXBone *dst, size_t count)
{
	for (size_t i = 0; i < count; ++i) {
		src = get_text(src, &dst[i].name_jp);
		src = get_text(src, &dst[i].name_en);
		src = get_field(src, dst[i].pos, sizeof(float), 3);
		src = get_field2(src, &dst[i].parent, header->bone_idx_size, sizeof(uint32_t), 1);
		src = get_field(src, &dst[i].transform_layer, sizeof(uint32_t), 1);
		src = get_field(src, &dst[i].flags, sizeof(uint16_t), 1);

		uint16_t flags = dst[i].flags;
		if (flags & BONE_FLAG_CONNECTED) 
			src = get_field2(src, &dst[i].tip.target, header->bone_idx_size, sizeof(uint32_t), 1);
		else
			src = get_field(src, &dst[i].tip.offset, sizeof(float), 3);
		
		if (flags & BONE_FLAG_LINK_ROTATION || flags & BONE_FLAG_LINK_MOVE) {
			src = get_field2(src, &dst[i].link.idx, header->bone_idx_size, sizeof(uint32_t), 1);
			src = get_field(src, &dst[i].link.rate, sizeof(float), 1);
		}

		if (flags & BONE_FLAG_FIXED_AXIS)
			src = get_field(src, &dst[i].fixed_axis, sizeof(float), 3);

		if (flags & BONE_FLAG_LOCAL_AXIS) {
			src = get_field(src, &dst[i].local_axis.x, sizeof(float), 3);
			src = get_field(src, &dst[i].local_axis.z, sizeof(float), 3);
		}

		if (flags & BONE_FLAG_EXT_PARENT_TRANSFORM)
			src = get_field(src, &dst[i].ext_parent_key, sizeof(uint32_t), 1);

		if (flags & BONE_FLAG_IK) {
			src = get_field2(src, &dst[i].ik.idx, header->bone_idx_size, sizeof(uint32_t), 1);
			src = get_field(src, &dst[i].ik.loop, sizeof(uint32_t), 1);
			src = get_field(src, &dst[i].ik.limit_angle, sizeof(float), 1);
			src = get_field(src, &dst[i].ik.link_count, sizeof(uint32_t), 1);
			
			size_t link_count = dst[i].ik.link_count;
			if (link_count > 0) {
				dst[i].ik.links = calloc(link_count, sizeof(PMXIKLink));
				src = pmx_parse_ik(src, header, dst[i].ik.links, link_count);
			}
		}	
	}

	return src;
}

static const char *pmx_parse_morph_offset(const char *src, const PMXHeader *header, PMXMorphOffset *dst, uint8_t type, size_t count)
{
	for (size_t i = 0; i < count; ++i) {
		switch (type) {
			case MORPH_TYPE_GROUP:
			case MORPH_TYPE_FLIP:
				src = get_field2(src, &dst[i].group_flip.idx, header->rb_idx_size, sizeof(uint32_t), 1);
				src = get_field(src, &dst[i].group_flip.rate, sizeof(uint8_t), 1);
				break;
			case MORPH_TYPE_VERTEX:
				src = get_field2(src, &dst[i].vertex.idx, header->vert_idx_size, sizeof(uint32_t), 1);
				src = get_field(src, &dst[i].vertex.offset, sizeof(float), 3);
				break;
			case MORPH_TYPE_BONE:
				src = get_field2(src, &dst[i].bone.idx, header->bone_idx_size, sizeof(uint32_t), 1);
				src = get_field(src, &dst[i].bone.move, sizeof(float), 3);
				src = get_field(src, &dst[i].bone.rotation, sizeof(float), 4);
				break;
			case MORPH_TYPE_UV:
			case MORPH_TYPE_ADD_UV_1:
			case MORPH_TYPE_ADD_UV_2:
			case MORPH_TYPE_ADD_UV_3:
			case MORPH_TYPE_ADD_UV_4:
				src = get_field2(src, &dst[i].uv.idx, header->vert_idx_size, sizeof(uint32_t), 1);
				src = get_field(src, &dst[i].uv.offset, sizeof(float), 4);
				break;
			case MORPH_TYPE_MATERIAL:
				src = get_field2(src, &dst[i].material.idx, header->mat_idx_size, sizeof(uint32_t), 1);
				src = get_field(src, &dst[i].material.method, sizeof(uint8_t), 1);
				src = get_field(src, &dst[i].material.diffuse, sizeof(float), 4);
				src = get_field(src, &dst[i].material.specular, sizeof(float), 3);
				src = get_field(src, &dst[i].material.power, sizeof(float), 1);
				src = get_field(src, &dst[i].material.ambient, sizeof(float), 3);
				src = get_field(src, &dst[i].material.edge, sizeof(float), 4);
				src = get_field(src, &dst[i].material.edge_size, sizeof(float), 1);
				src = get_field(src, &dst[i].material.tex_tint, sizeof(float), 4);
				src = get_field(src, &dst[i].material.env_tint, sizeof(float), 4);
				src = get_field(src, &dst[i].material.toon_tint, sizeof(float), 4);
				break;
			case MORPH_TYPE_IMPULSE:
				src = get_field2(src, &dst[i].impulse.idx, header->rb_idx_size, sizeof(uint32_t), 1);
				src = get_field(src, &dst[i].impulse.local, sizeof(uint8_t), 1);
				src = get_field(src, &dst[i].impulse.velocity, sizeof(float), 3);
				src = get_field(src, &dst[i].impulse.torque, sizeof(float), 3);
				break;
			default:
				return NULL;
		}
	}

	return src;
}

static const char *pmx_parse_morph(const char *src, const PMXHeader *header, PMXMorph *dst, size_t count)
{
	for (size_t i = 0; i < count; ++i) {
		src = get_text(src, &dst[i].name_jp);
		src = get_text(src, &dst[i].name_en);
		src = get_field(src, &dst[i].panel, sizeof(uint8_t), 1);
		src = get_field(src, &dst[i].type, sizeof(uint8_t), 1);
		src = get_field(src, &dst[i].offset_count, sizeof(uint32_t), 1);
		uint32_t offset_count = dst[i].offset_count;
		if (offset_count > 0) {
			dst[i].offsets = malloc(offset_count * sizeof(PMXMorphOffset));
			assert(dst[i].offsets);
			src = pmx_parse_morph_offset(src, header, dst[i].offsets, dst[i].type, dst[i].offset_count);	
		}	
	}

	return src;
}

static const char *pmx_parse_frame_elem(const char *src, const PMXHeader *header, PMXFrameElement *dst, size_t count)
{
	for (size_t i = 0; i < count; ++i) {
		src = get_field(src, &dst[i].type, sizeof(uint8_t), 1);
		switch (dst[i].type) {
		case FRAME_ELEM_TYPE_BONE:
			src = get_field2(src, &dst[i].idx, header->bone_idx_size, sizeof(uint32_t), 1);
			break;
		case FRAME_ELEM_TYPE_MORPH:
			src = get_field2(src, &dst[i].idx, header->morph_idx_size, sizeof(uint32_t), 1);
			break;
		default:
			return NULL;
		}
	}

	return src;
}

static const char *pmx_parse_frame(const char *src, const PMXHeader *header, PMXFrame *dst, size_t count)
{
	for (size_t i = 0; i < count; ++i) {
		src = get_text(src, &dst[i].name_jp);
		src = get_text(src, &dst[i].name_en);
		src = get_field(src, &dst[i].special, sizeof(uint8_t), 1);
		src = get_field(src, &dst[i].elem_count, sizeof(uint32_t), 1);
		uint32_t elem_count = dst[i].elem_count;
		if (elem_count > 0) {
			dst[i].elems = malloc(elem_count * sizeof(PMXFrameElement));
			assert(dst[i].elems);
			src = pmx_parse_frame_elem(src, header, dst[i].elems, elem_count);
		}
	}

	return src;
}

static const char *pmx_parse_rigidbody(const char *src, const PMXHeader *header, PMXRigidBody *dst, size_t count)
{
	for (size_t i = 0; i < count; ++i) {
		src = get_text(src, &dst[i].name_jp);
		src = get_text(src, &dst[i].name_en);
		src = get_field2(src, &dst[i].bone_idx, header->bone_idx_size, sizeof(uint32_t), 1);
		src = get_field(src, &dst[i].group, sizeof(uint8_t), 1);
		src = get_field(src, &dst[i].no_collide_group, sizeof(uint16_t), 1);
		src = get_field(src, &dst[i].shape, sizeof(uint8_t), 1);
		src = get_field(src, &dst[i].shape_size, sizeof(float), 3);
		src = get_field(src, &dst[i].pos, sizeof(float), 3);
		src = get_field(src, &dst[i].rot, sizeof(float), 3);
		src = get_field(src, &dst[i].mass, sizeof(float), 1);
		src = get_field(src, &dst[i].move_decay, sizeof(float), 1);
		src = get_field(src, &dst[i].rot_decay, sizeof(float), 1);
		src = get_field(src, &dst[i].elastic, sizeof(float), 1);
		src = get_field(src, &dst[i].friction, sizeof(float), 1);
		src = get_field(src, &dst[i].type, sizeof(uint8_t), 1);
	}

	return src;
}

static const char *pmx_parse_joint(const char *src, const PMXHeader *header, PMXJoint *dst, size_t count)
{
	for (size_t i = 0; i < count; ++i) {
		src = get_text(src, &dst[i].name_jp);
		src = get_text(src, &dst[i].name_en);
		src = get_field(src, &dst[i].type, sizeof(uint8_t), 1);
		src = get_field2(src, &dst[i].idx1, header->rb_idx_size, sizeof(uint32_t), 1);
		src = get_field2(src, &dst[i].idx2, header->rb_idx_size, sizeof(uint32_t), 1);
		src = get_field(src, &dst[i].pos, sizeof(float), 3);
		src = get_field(src, &dst[i].rot, sizeof(float), 3);
		src = get_field(src, &dst[i].pos_limit.lower, sizeof(float), 3);
		src = get_field(src, &dst[i].pos_limit.upper, sizeof(float), 3);
		src = get_field(src, &dst[i].rot_limit.lower, sizeof(float), 3);
		src = get_field(src, &dst[i].rot_limit.upper, sizeof(float), 3);
		src = get_field(src, &dst[i].spring_pos, sizeof(float), 3);
		src = get_field(src, &dst[i].spring_rot, sizeof(float), 3);
	}

	return src;
}

int pmx_parse(const char *src, PMXModel *dst)
{
	TRACE(" ********** PMX Parser **********\n");
	if (strncmp(src, "PMX ", 4)) {
		snprintf(error_msg, ERROR_MSG_LEN, "Not a pmx file\n");
		return -1;
	}
	memcpy(&dst->header, src, sizeof(dst->header));
	if (dst->header.uv_count > 0) {
		snprintf(error_msg, ERROR_MSG_LEN, "Additional UV is unsupported\n");
		return -1;
	}
	src += sizeof(dst->header);
	src = pmx_parse_info(src, &dst->info);

	dst->vertices = NULL;
	dst->faces = NULL;
	dst->textures = NULL;
	dst->materials = NULL;
	dst->bones = NULL;
	dst->morphs = NULL;
	dst->frames = NULL;
	dst->rigidbodies = NULL;
	dst->joints = NULL;

	src = get_field(src, &dst->vertex_count, sizeof(dst->vertex_count), 1);
	TRACE("Vertex count: %u\n", dst->vertex_count);
	dst->vertices = calloc(dst->vertex_count, sizeof(PMXVert));
	src = pmx_parse_vert(src, &dst->header, dst->vertices, dst->vertex_count);
	if (!src) {
		snprintf(error_msg, ERROR_MSG_LEN, "Failed to parse vertices\n");
		return -1;
	}

	src = get_field(src, &dst->face_count, sizeof(dst->face_count), 1);
	dst->face_count /= 3;
	TRACE("Face count: %u\n", dst->face_count);
	dst->faces = calloc(dst->face_count, sizeof(PMXFace));
	src = pmx_parse_face(src, &dst->header, dst->faces, dst->face_count);
	if (!src) {
		snprintf(error_msg, ERROR_MSG_LEN, "Failed to parse faces\n");
		return -1;
	}

	src = get_field(src, &dst->texture_count, sizeof(dst->texture_count), 1);
	TRACE("Texture count: %u\n", dst->texture_count);
	dst->textures = calloc(dst->texture_count, sizeof(PMXTex));
	src = pmx_parse_tex(src, dst->textures, dst->texture_count);
	if (!src) {
		snprintf(error_msg, ERROR_MSG_LEN, "Failed to parse textures\n");
		return -1;
	}

	src = get_field(src, &dst->material_count, sizeof(dst->material_count), 1);
	TRACE("Material count: %u\n", dst->material_count);
	dst->materials= calloc(dst->material_count, sizeof(PMXMat));
	src = pmx_parse_mat(src, &dst->header, dst->materials, dst->material_count);
	if (!src) {
		snprintf(error_msg, ERROR_MSG_LEN, "Failed to parse materials\n");
		return -1;
	}
	
	src = get_field(src, &dst->bone_count, sizeof(dst->bone_count), 1);
	TRACE("Bone count: %u\n", dst->bone_count);
	dst->bones = calloc(dst->bone_count, sizeof(PMXBone));
	src = pmx_parse_bone(src, &dst->header, dst->bones, dst->bone_count);
	if (!src) {
		snprintf(error_msg, ERROR_MSG_LEN, "Failed to parse bones\n");
		return -1;
	}

	src = get_field(src, &dst->morph_count, sizeof(dst->morph_count), 1);
	TRACE("Morph count: %u\n", dst->morph_count);
	dst->morphs = calloc(dst->morph_count, sizeof(PMXMorph));
	src = pmx_parse_morph(src, &dst->header, dst->morphs, dst->morph_count);
	if (!src) {
		snprintf(error_msg, ERROR_MSG_LEN, "Failed to parse bones\n");
		return -1;
	}

	src = get_field(src, &dst->frame_count, sizeof(dst->frame_count), 1);
	TRACE("Frame count: %u\n", dst->frame_count);
	dst->frames = calloc(dst->frame_count, sizeof(PMXFrame));
	src = pmx_parse_frame(src, &dst->header, dst->frames, dst->frame_count);
	if (!src) {
		snprintf(error_msg, ERROR_MSG_LEN, "Failed to parse frames\n");
		return -1;
	}

	src = get_field(src, &dst->rigidbody_count, sizeof(dst->rigidbody_count), 1);
	TRACE("Rigidbody count: %u\n", dst->rigidbody_count);
	dst->rigidbodies = calloc(dst->rigidbody_count, sizeof(PMXRigidBody));
	src = pmx_parse_rigidbody(src, &dst->header, dst->rigidbodies, dst->rigidbody_count);
	if (!src) {
		snprintf(error_msg, ERROR_MSG_LEN, "Failed to parse rigidbodies\n");
		return -1;
	}

	src = get_field(src, &dst->joint_count, sizeof(dst->joint_count), 1);
	TRACE("Joint count: %u\n", dst->joint_count);
	dst->joints = calloc(dst->joint_count, sizeof(PMXJoint));
	src = pmx_parse_joint(src, &dst->header, dst->joints, dst->joint_count);
	if (!src) {
		snprintf(error_msg, ERROR_MSG_LEN, "Failed to parse joints\n");
		return -1;
	}
	
	TRACE(" ********** Parse OK **********\n");
	return 0;
}

void pmx_free(PMXModel *model)
{
	if (model->vertices) {
		free(model->vertices);
		model->vertices = NULL;
	}

	if (model->faces) {
		free(model->faces);
		model->faces = NULL;
	}

	if (model->textures) {
		free(model->textures);
		model->textures = NULL;
	}

	if (model->materials) {
		free(model->materials);
		model->materials = NULL;
	}
	
	if (model->bones) {
		for (size_t i = 0; i < model->bone_count; ++i)
			if (model->bones[i].ik.links)
				free(model->bones[i].ik.links);
		free(model->bones);
		model->bones = NULL;
	}

	if (model->morphs) {
		for (size_t i = 0; i < model->morph_count; ++i)
			if (model->morphs[i].offsets)
				free(model->morphs[i].offsets);
		free(model->morphs);
		model->morphs = NULL;
	}

	if (model->frames) {
		for (size_t i = 0; i < model->frame_count; ++i)
			if (model->frames[i].elems)
				free(model->frames[i].elems);
		free(model->frames);
		model->frames = NULL;
	}

	if (model->rigidbodies) {
		free(model->rigidbodies);
		model->rigidbodies = NULL;
	}

	if (model->joints) {
		free(model->joints);
		model->joints = NULL;
	}
}

const char *pmx_get_error_msg(void)
{
	return error_msg;
}


