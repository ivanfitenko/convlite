#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <gd.h>

#include "commons.h"

int gd_broken_dimensions (char *file, int *sx, int *sy) {
	FILE *in;
	gdImagePtr im;
	in = fopen (file, "rb");
	im=gdImageCreateFromJpeg(in);
	*sx=gdImageSX(im);
	*sy=gdImageSY(im);
	if (!*sx || !*sy)
		return 1;
	if (im)
		gdImageDestroy(im);
	fclose(in);
	return 0;
}
