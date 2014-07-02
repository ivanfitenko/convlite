#include <assert.h>
#include <errno.h>
#include <stdbool.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <sys/stat.h>
#include <libnsgif.h>
#include <sys/types.h>
#include <stdlib.h>
#include <gd.h>

#include "commons.h"
#include "gifthumb.h"

FILE *myfd;
unsigned char *dataframe;
unsigned char *load_file(const char *path, size_t *data_size);
void warning(const char *context, int code);
void *bitmap_create(int width, int height);
void bitmap_set_opaque(void *bitmap, bool opaque);
bool bitmap_test_opaque(void *bitmap);
unsigned char *bitmap_get_buffer(void *bitmap);
void bitmap_destroy(void *bitmap);
void bitmap_modified(void *bitmap);
unsigned char *pull_gif_frame(unsigned char *full_gif, unsigned long frame_num,
                                size_t *frame_size);
unsigned long header_offset, cur_offset;
char *cur_filename;
unsigned char *cur_dataframe;
void* cur_dataframe_buffer;
gif_animation gif;
size_t size;


int resize_gif(char *outfile, char *file, int tw, int th, int scale) {
	FILE *out;
	int i;
	int transp_orig;
	int transp_new = -1;
	size_t frame_size = 0;
	int disp_method = 0;
	gdImagePtr im;
	gdImagePtr im_new;
	/* initialize libnsgif */
	gif_bitmap_callback_vt bitmap_callbacks = {
                bitmap_create,
                bitmap_destroy,
                bitmap_get_buffer,
                bitmap_set_opaque,
                bitmap_test_opaque,
                bitmap_modified
    };
	gif_result code;
	/* prepare structure to write decoded data */
	gif_create(&gif, &bitmap_callbacks);
	/* load file into memory */
    unsigned char *data = load_file(file, &size);
	/* decode frame data */
	/* Note that we triple-store data: full gif in *data, */
	/* structured in *gif and gd-formatted in im. At least*/
	/* bitmap data in *gif may need to  be cleared */
    do {
     	code = gif_initialise(&gif, size, data);
    	if (code != GIF_OK && code != GIF_WORKING) {
    		warning("gif_initialise", code);
	        /* clean up libnsgif */
		gif_finalise(&gif);
		free(data);
     		return code;
    	}
    } while (code != GIF_OK);
   /* initialise storage for gd original and resized gifs */
    out = fopen(outfile, "wb");
    if (!out) {
	fprintf(stderr,
		"Unable to write to output file -- exiting\n");
		exit(1);
    }

    if (scale==SHRINK) {
	if ( (tw>=gif.width) && (th>=gif.height)) {
		th=gif.height;
		tw=gif.width;
	} else {
		scale = ASPECT;
	}
    }
    if (scale==FIT) {
	if ((float)gif.width/(float)gif.height*(float)th > tw)
	    tw=(float)gif.width/(float)gif.height*(float)th;
	else
	    th=(float)gif.height/(float)gif.width*(float)tw;
    }
    if (scale == ASPECT) {
	if ((float)gif.width/(float)gif.height*(float)th < tw)
		tw=(float)gif.width/(float)gif.height*(float)th;
	else
		th=(float)gif.height/(float)gif.width*(float)tw;
    }
    /* (scale == EXACT) just requires no changes on target dimensions */
    
    for (i=1; i<=gif.frame_count; i++) {
	/* cut the frame into gd structure */
	/* for some reason, gd shifts the pointer, so we need to use buffer*/
	cur_dataframe=pull_gif_frame(data, (unsigned long) i, &frame_size);
	cur_dataframe_buffer=malloc(frame_size);
	memmove(cur_dataframe_buffer,cur_dataframe,frame_size);
	im=gdImageCreateFromGifPtr(frame_size,(void*) cur_dataframe_buffer);
	if (!im) {
	    fprintf (stderr, "gd failed to process libnsgif frame\n");
	    return -1;
	}
	free(cur_dataframe_buffer);
	im_new=gdImageCreate(tw, th);
	/*original image transparency index */
	transp_orig = gdImageGetTransparent(im);
	if (transp_orig != (-1))
	    gdImageColorTransparent(im, transp_orig);
	else
	    gdImageColorTransparent(im, -1);
	/*scale by copying*/
	gdImageCopyResampled(im_new, im, 0, 0, 0, 0, tw, th, gif.width, gif.height);
	/* try to find closet transparency index without extending the palette */
	if (transp_orig != -1) {
	    if ((transp_new=gdImageColorExact(im,gdImageRed(im,transp_orig),
		    gdImageGreen(im,transp_orig),gdImageBlue(im,transp_orig))) < 0)
		transp_new=gdImageColorClosest(im,gdImageRed(im,transp_orig),
		gdImageGreen(im,transp_orig),gdImageBlue(im,transp_orig));
		gdImageColorTransparent(im_new, transp_new);
	}
	/* write animations for mutiple frames */ 
	if (gif.frame_count > 1) {
	    /* convert libnsgif frame disposal to gd disposal */
	    switch (gif.frames[i].disposal_method) {
				case 1:
					disp_method = 1;
					break;
				case 2:
					disp_method = 3;
					break;
				case 3:
					disp_method = 2;
					break;
				default:
					disp_method = 0;
					break;
	    }
	    if (i == 1)
		/*global color palette gives better sizes but may loose frames*/
		gdImageGifAnimBegin(im_new, out, 1,0 );
	    gdImageGifAnimAdd(im_new, out, 1, 0, 0, gif.frames[i-1].frame_delay, disp_method, NULL);
	    if (i == gif.frame_count) 
		gdImageGifAnimEnd(out);
	} else 
	    gdImageGif(im_new, out);
	if (im) 
	    gdImageDestroy(im);
	if (im_new)
	    gdImageDestroy(im_new);
    }
    fclose(out);
    /* clean up libnsgif */
    gif_finalise(&gif);
    free(data);
    return 0;
}

unsigned char *pull_gif_frame(unsigned char *full_gif, unsigned long frame_num,
                                size_t *frame_size) {
        unsigned char *req_frame, *req_frame_buff;
        unsigned long frame_end;
        /* the last frame offset is not reported. Assume the end of the data */
        if (frame_num == gif.frame_count)
                frame_end = size;
        else
                frame_end=gif.frames[frame_num].frame_pointer;
        /* compose the image of header (frame0) and the image data */
        *frame_size=(size_t) (frame_end-gif.frames[frame_num-1].frame_pointer);
        req_frame_buff=malloc(*frame_size+gif.frames[0].frame_pointer);
        memmove(req_frame_buff,full_gif,(size_t) gif.frames[0].frame_pointer);
        req_frame_buff+=gif.frames[0].frame_pointer;
        memmove(req_frame_buff,full_gif+gif.frames[frame_num-1].frame_pointer,
        		*frame_size);
        /* cleanup: count frame size together with header, shift the pointer */
        /* back, free the memory */
        *frame_size=*frame_size + ((size_t) gif.frames[0].frame_pointer);
        req_frame=req_frame_buff-gif.frames[0].frame_pointer;
        free(req_frame_buff-gif.frames[0].frame_pointer);
        return req_frame;
}

unsigned char *load_file(const char *path, size_t *data_size) {
        FILE *fd;
        struct stat sb;
        unsigned char *buffer;
        size_t size;
        size_t n;

        fd = fopen(path, "rb");
        if (!fd) {
                perror(path);
                exit(EXIT_FAILURE);
        }
        if (stat(path, &sb)) {
                perror(path);
                exit(EXIT_FAILURE);
        }
        size = sb.st_size;

        buffer = malloc(size);
        if (!buffer) {
                fprintf(stderr, "Unable to allocate %lld bytes\n",
                                (long long) size);
                exit(EXIT_FAILURE);
        }

        n = fread(buffer, 1, size, fd);
        if (n != size) {
                perror(path);
                exit(EXIT_FAILURE);
        }

        fclose(fd);

        *data_size = size;
        return buffer;
}


void warning(const char *context, gif_result code) {
        fprintf(stderr, "%s failed: ", context);
        switch (code) {
        case GIF_INSUFFICIENT_FRAME_DATA:
                fprintf(stderr, "GIF_INSUFFICIENT_FRAME_DATA");
                break;
        case GIF_FRAME_DATA_ERROR:
                fprintf(stderr, "GIF_FRAME_DATA_ERROR");
                break;
        case GIF_INSUFFICIENT_DATA:
                fprintf(stderr, "GIF_INSUFFICIENT_DATA");
                break;
        case GIF_DATA_ERROR:
                fprintf(stderr, "GIF_DATA_ERROR");
                break;
        case GIF_INSUFFICIENT_MEMORY:
                fprintf(stderr, "GIF_INSUFFICIENT_MEMORY");
                break;
        default:
                fprintf(stderr, "unknown code %i", code);
                break;
        }
        fprintf(stderr, "\n");
}


void *bitmap_create(int width, int height) {
        return calloc(width * height, 4);
}


void bitmap_set_opaque(void *bitmap, bool opaque) {
        (void) opaque;  /* unused */
        assert(bitmap);
}


bool bitmap_test_opaque(void *bitmap) {
        assert(bitmap);
        return false;
}


unsigned char *bitmap_get_buffer(void *bitmap) {
        assert(bitmap);
        return bitmap;
}


void bitmap_destroy(void *bitmap) {
        assert(bitmap);
        free(bitmap);
}


void bitmap_modified(void *bitmap) {
        assert(bitmap);
        return;
}

