#include "pmx_model.h"

#include <stdio.h>
#include <stdlib.h>

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

	char *raw = (char *)malloc(filesize);
	if (!raw) {
		fprintf(stderr, "ERROR: Could not allocate memory\n");
		exit(EXIT_FAILURE);
	}

	if (fread(raw, sizeof(char), filesize, fp) != filesize) {
		fprintf(stderr, "ERROR: Could not read %s\n", filepath);
		exit(EXIT_FAILURE);
	}
	
	PMXModel model;
	if (pmx_parse(raw, &model)) {
		fprintf(stderr, "ERROR: Failed to parse raw.\nMessage: %s", error_msg);
		exit(EXIT_FAILURE);
	}

	pmx_free(&model);
	free(raw);

	return 0;
}

