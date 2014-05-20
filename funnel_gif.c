/*
* WARNING: the file is full of dirty code and is generally unsafe.
* It needs a rewrite to automatically determine variable sizes and efficiently
* check for authenticity of the files it accesses.
* Also, the crash-tests are still to be done.
*/

#define X_DISPLAY_MISSING

#include <Imlib2.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>

#include <gd.h>


int funnel_gif(char *outfile) {
    /* max path length on linux, AFAIR. Seems to be an overkill, but safe */
    char tmp_fd_path[4096];
    char tmp_filename[4096] = "";
//    char tmp_filename[21] = "";
    gdImagePtr im_new;
    
    Imlib_Load_Error imlib_error;
    
    FILE *tmpfile, *out;
    int fd = -1;

    /* create a temporary file to write a png */
    strncpy(tmp_filename, "/tmp/convlite.XXXXXX", sizeof tmp_filename);
// FIXME don't fdopen until dirty deeds are done
    if ((fd = mkstemp(tmp_filename)) == -1 ||
	    (tmpfile = fdopen(fd, "w+")) == NULL) {
	if (fd != -1) {
	    unlink(tmp_filename);
	    close(fd);
	}
	fprintf(stderr, "%s: %s\n", tmp_filename, strerror(errno));
        return (-1);
    }

    sprintf(tmp_fd_path, "/proc/self/fd/%d", fd);
//FIXME readlink doesnt terminate the string; must append a null-byte here
    readlink(tmp_fd_path, tmp_filename, sizeof(tmp_filename));

    /* 
    * work with the context already set in imthumb.c 
    * use png as a format suitable for gd lib to open.
    */
    imlib_image_set_format("png");
    imlib_save_image_with_error_return (tmp_filename, &imlib_error);
    if (imlib_error) {
        fprintf (stderr, "Cannot write a temporary file, imlib error %d\n",
    		 imlib_error);
        return imlib_error;
    }

    /* gd part: open our intermediate temporary file and save its data as gif */
    im_new = gdImageCreateFromPng(tmpfile);

    /* read everything into memory, the temporary file is not needed anymore */
    fclose (tmpfile);
    unlink (tmp_filename);

    if (!im_new) {
        fprintf(stderr, 
		"Unable to read in a temporary file when converting gif\n");
	return (-1);
    }
    out = fopen(outfile, "wb");
    if (!out) {
	fprintf(stderr, "Unable to write to output file -- exiting\n");
	return (-1);
    }
    
    /* cleanup */
    gdImageGif(im_new, out);
    gdImageDestroy(im_new);
    fclose (out);
    return 0;
}

