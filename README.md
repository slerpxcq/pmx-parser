# pmx-parser
PMX model parser in C\
See [pmx_model.h](pmx_model.h) for struct definitions.
## Usage
```C
PMXModel model;
if (pmx_parse(raw, &model)) {
	fprintf(stderr, "ERROR: Parse failed.\nMessage: %s", pmx_get_error_msg());
	exit(EXIT_FAILURE);
}

// Do operations...

pmx_free(&model);
```
