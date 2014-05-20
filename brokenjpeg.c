#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <gd.h>

#include "fallbacks.h"
#include "commons.h"

int resize_brokenjpeg(char *outfile, char *file, int tw, int th, int scale) {
	FILE *out, *in;
	gdImagePtr im;
	gdImagePtr im_new;
   	/* initialise storage for gd original and resized gifs */
	in = fopen (file, "rb");
	out = fopen(outfile, "wb");
	if (!out) {
		fprintf(stderr,
			"Unable to write to output file -- exiting\n");
			exit(1);
	}
	im=gdImageCreateFromJpeg(in);
	if (scale==NONE) {
	    th=gdImageSY(im);
	    tw=gdImageSX(im);
	}
	if (scale==SHRINK) {
		if ( (tw>=gdImageSX(im)) && (th>=gdImageSY(im))) {
			th=gdImageSY(im);
			tw=gdImageSX(im);
		} else {
			scale = ASPECT;
		}
	}
	if (scale==FIT) {
		if ((float)gdImageSX(im)/(float)gdImageSY(im)*(float)th > tw)
			tw=(float)gdImageSX(im)/(float)gdImageSY(im)*(float)th;
		else
			th=(float)gdImageSY(im)/(float)gdImageSX(im)*(float)tw;
	}
	if (scale == ASPECT) {
		if ((float)gdImageSX(im)/(float)gdImageSY(im)*(float)th < tw)
			tw=(float)gdImageSX(im)/(float)gdImageSY(im)*(float)th;
		else
			th=(float)gdImageSY(im)/(float)gdImageSX(im)*(float)tw;
	}
	im_new=gdImageCreateTrueColor(tw, th);
	/*scale by copying*/
	gdImageCopyResampled(im_new, im, 0, 0, 0, 0, tw, th, gdImageSX(im), gdImageSY(im));
	/* try to find closet transparency index without extending the palette */
	gdImageJpeg(im_new, out, -1);
	if (im) 
		gdImageDestroy(im);
	if (im_new)
		gdImageDestroy(im_new);
	fclose(in);
	fclose(out);
	/* All's well that ends well. */
	return 0;
}
