#define X_DISPLAY_MISSING

#include <Imlib2.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>

#include <gd.h>

#include "imlib_errno_generated.h"
#include "exif_data.h"

#include "commons.h"
#include "fallbacks.h"
#include "gifthumb.h"
#include "copy.h"
#include "funnel_gif.h"
#include "brokenjpeg.h"

void orient_imlib_exif(char *exif_infile);

int convert_imlib(char *outfile, char *file, int tw, int th, char *ext,
					 int scale, int auto_orient) {
    int sw, sh;
    char *in_fmt, *fmt;
    /* have it on stack so that no malloc-free games are needed */
    char *jpeg_fmt = "jpeg";
    Imlib_Image in_img, out_img;
    Imlib_Load_Error imlib_error;
    /* gd2 will not be able to access an image cached by imlib2 */
    in_img = imlib_load_image_without_cache(file);
    if (!in_img) {
        fprintf(stderr, "Unable to load %s\n", file);
        return 1;
    }
    imlib_context_set_image(in_img);

    in_fmt = imlib_image_format();
    
    /* JPEG format sanitization: set all the various extensions to mean */
    /* the same format */
    if ( (!strncasecmp(ext, "jpg", strlen(in_fmt))) ||
	    (!strncasecmp(ext, "jpe", strlen(in_fmt))) )
	fmt = jpeg_fmt;
    else
	fmt = ext;

    /* some kinds of images/combinations require special processing */

    /* When used for batch processing, it is possible that input file */
    /* requires no conversion or resizing. If so, just copy it. */
    if ( (strlen(in_fmt) == strlen(fmt)) 
		&& (strncasecmp(in_fmt, fmt, strlen(in_fmt)) == 0) 
		&& (scale == NONE) )
	return copy_unmodified (file, outfile);
    
    /* gif to gif conversion should preserve animation, so use libnsgif */
    if ( (strncasecmp("gif", in_fmt, 3) == 0) 
	    && (strncasecmp("gif", fmt, 3) ==0) ) {
	/*
	* cleanup imlib2 memory so that gd2 doesn't stump upon it -  
	* and it _will_ try, and thus fail to read it. Dunno why
	*/
	imlib_free_image();
	return resize_gif(outfile, file, tw, th, scale);
    }
    /* 32-bit JPEG conversion is only available when both source and target are
     * JPEG format images.
    */
    if (strncasecmp("brokenjpeg", fmt, 10) == 0) {
	if (strncasecmp("jpeg", in_fmt, 3) == 0)
	    return resize_brokenjpeg(outfile, file, tw, th, scale);
	else 
//FIXME: once pointer-to-gd funnelling is completed, support 32-bit jpeg-to-any
	    /* for non-jpeg source, convert jpeg with imlib */
	    fmt=jpeg_fmt;
    }
    
    /* orient image according to exif data */
    /* currently, only jpeg is supported because I have no tiffs to test */
    if ( auto_orient && (strncasecmp("jpeg", in_fmt, 3) == 0) )
	/* we dont't care whether orientation is successful or not */ 
	orient_imlib_exif(file); 

    /* for the rest of the combinations, imlib can process the source image */
    sw=imlib_image_get_width();
    sh=imlib_image_get_height();
    if (!sw || !sh) {
    	if (gd_broken_dimensions(file, &sw, &sh)) {
    		fprintf(stderr, "Cannot determine source dimensions\n");
    		return 1;
    	}
    }

    /*
    * For most formats, conversion without scaling can be done by just setting
    * the file with imlib_image_set_format on the original imlib_image and 
    * saving the file with imlib_save_image. However, this wat imlib would use
    * a 'hypothethetical' P8 format for PPM files to preserve transparency.
    * ImageMagick's convert, on the other hand, would save images as P8 with
    * transparency lost. In imlib2, alpha channel cannot be dropped from
    * original imlib_image (btw, why???), so we need to create a new image
    * even if no resizing is required.
    */
    if ((scale != NONE) || (strncasecmp("pnm", fmt, 3) == 0) 
	|| (strncasecmp("ppm", fmt, 3) == 0) ){
	    if (scale==SHRINK) {
    		if ((tw>=sw) && (th>=sh)) {
        	    th=sh;
        	    tw=sw;
		} else {
		    scale = ASPECT;
		}
	    }
	    if (scale==FIT) {
		    if ((float)sw/(float)sh*(float)th > tw)
			tw=(float)sw/(float)sh*(float)th;
		    else
			th=(float)sh/(float)sw*(float)tw;
	    }
	    if (scale == ASPECT) {
		    if ((float)sw/(float)sh*(float)th < tw) 
			tw=(float)sw/(float)sh*(float)th;
		    else
			th=(float)sh/(float)sw*(float)tw;
	    }
	    /* (scale == EXACT) just requires no changes on target dimensions */

	    if ( (strncasecmp("ppm", fmt, 3) == 0)
		|| (strncasecmp("pnm", fmt, 3) == 0) ) {
		    if (scale == NONE) {
			tw=sw;
			th=sh;
		    }
	    }

	    out_img = imlib_create_cropped_scaled_image(0, 0, sw, sh, tw, th );
	    if (!out_img) {
    		fprintf(stderr, "Failed to create scaled image\n");
    		return 1;
    	    }

	    imlib_context_set_image(out_img);
    }

    if ( (strncasecmp("pnm", fmt, 3) == 0) ||
	(strncasecmp("ppm", fmt, 3) == 0)) {
	imlib_image_set_has_alpha(0);
    }
    /* imlib2 cannot write gif output, so we will use GD here.
    * On the other side, gd supports less formats than imlib2 does,
    * so convert imlib's work to PNG before funnelling it to gd.
    */
    if (strncasecmp("gif", fmt, 3) == 0) {
	return funnel_gif(outfile);
    } else {
	imlib_image_set_format(fmt);
	imlib_save_image_with_error_return (outfile, &imlib_error);
	if (imlib_error) {
	    if ( (imlib_error>=0) && (imlib_error < GEN_IMLIB_ERRNO_LENGTH) )
		fprintf (stderr, "Failure, imlib error code %d: %s\n", \
					    imlib_error, \
					    imlib_errno_generated[imlib_error]);
	    else
		fprintf (stderr, "Failure, imlib error code %d: %s\n", \
					    imlib_error, \
					    "UNLISTED_ERROR");
	}
    }
    return imlib_error;
}

void orient_imlib_exif(char *exif_infile) {
    int orientation;
    orientation = exif_orientation(exif_infile);
    switch (orientation) {
	    case 0:
		/* missing orientation */
		break;
	    case 1:
		/* normal orientation */
		break;
	    case 2:
		/* mirrored orientation */
		imlib_image_flip_horizontal();
		break;
	    case 3:
		/* camera upside down, rotate 180 degrees */
		imlib_image_orientate(2);
		break;
	    case 4:
		/* upside down and mirrored, rotate 180 degrees and flip */
		imlib_image_orientate(2);
		imlib_image_flip_horizontal();
		break;
	    case 5:
		/* 
		* portrait, mirrored, upside down. Rotate 90 counter-clockwise
		* and flip
		*/
		imlib_image_orientate(3);
		imlib_image_flip_horizontal();
		break;
	    case 6:
		/* portrait, rightside-left. Rotate 90 degrees clockwise */
		imlib_image_orientate(1);
		break;
	    case 7:
		/* portrait, mirrored. Rotate 90 clockwise and flip*/
		imlib_image_orientate(1);
		imlib_image_flip_horizontal();
		break;
	    case 8:
		/* portrait. Rotate 90 counter-clockwise */
		imlib_image_orientate(3);
		break;
	    default:
		break;
    }
    return;
}
