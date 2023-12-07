#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>

#define NDEBUG
#ifdef NDEBUG
#define DBG_LOG(x, ...) fprintf(stderr, (x), __VA_ARGS__)
#else
#define DBG_LOG(x, ...)
#endif

#define ERROR_MSG_LEN 128

typedef struct 
{
				
} PMXModel;

static PMXModel parsed_model;
static char *model;
static char error_msg[ERROR_MSG_LEN];

uint8_t pmx_parse(const char *raw, PMXModel *parsed)
{
	if (strncmp(raw, "PMX ", 4)) {
		snprintf(error_msg, ERROR_MSG_LEN, "Not a pmx file\n");
		return 1;
	}

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
	
	if (pmx_parse(model, &parsed_model)) {
		fprintf(stderr, "ERROR: Failed to parse model.\nMessage: %s", error_msg);
		exit(EXIT_FAILURE);
	}

	free(model);

	return 0;
}

