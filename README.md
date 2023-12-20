# pmx-parser
PMX model parser in C
## Usage
```C
	PMXModel model;
   if (pmx_parse(raw, &model)) {
		fprintf(stderr, "ERROR: Failed to parse raw.\nMessage: %s", pmx_get_error_msg());
		exit(EXIT_FAILURE);
	}

    // ...

    pmx_free(&model);
```
