#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include "commons.h"
#include "imthumb.h"
#include "options.h"

void usage();
char * strip_framenum(char *filename);

int main(int argc, char *argv[]) {
	char *file;
	char *outfile;
	char *format_ext;
	char brokenjpeg_fmt[11];
	char *dims;
	int delim_pos;
	int tw, th, scale;
	int have_dims, have_resize, have_broken, have_orient;
	int i = 1;
	int j = 1;

	/* only successful conversions will set return code to 0 */
	int c_exitcode = -1;
	
	/* 
	* an overkill, but my brain just burnt while debuggin this variable.
	* So let's have clean and save tiny OVERKILL
	*/
	strncpy (brokenjpeg_fmt, "brokenjpeg", 10);
	brokenjpeg_fmt[10] = '\0';
	
	dims = NULL;
	file = outfile = NULL;
	have_broken = have_dims = have_resize = have_orient = 0;
	tw = th = scale = 0;


	/* parse options */
	if (!argv[1])
			usage();
	do {
		/*args without dashes are expected to be a filename for input or output*/
		/* but silently skip ignored ones */
		if (!strncmp(argv[i],"registry:",9)) {
			i++;
			continue;
		}
		if (strncmp(argv[i],"-",1)) {
			if (!file) {
				file=strip_framenum(argv[i]);
			} else if (!outfile) {
				outfile=argv[i];
			} else {
				usage();
			}
		/* args having dashes are options */
		} else {
			while (option[j].opt_num) {
				if (!strncmp(argv[i],option[j].argnam,strlen(option[j].argnam))) {
					switch (option[j].opt_num) {
						case CNVL_OPT_RESIZE: 
							/* resize option */
							dims=argv[i+1];
							have_dims++;
							have_resize++;
							i++;
							break;
						case CNVL_OPT_BROKEN: 
							/* broken 32bit jpeg */
							have_broken=1;
							break;
						case CNVL_OPT_AUTOORIENT: 
							/*
							* auto orient based on
							* exif info
							*/ 
							have_orient = 1;
							break;
						case CNVL_OPT_IGNORE_TWO: 
							/* 
							* ignored option, 
							* plus skip its value
							*/
							i++;
						case CNVL_OPT_IGNORE: 
							/* 
							* ignored option 
							* Here for readability
							*/
							break;
						default:
							break;
					}
				}
				j++;
			}
		}
		i++;
		j=1;
	} while (i != argc);
	if (have_resize) {
	    /* resize parameter and dimensions must be passed only once */
	    if ((have_dims!=1) || (have_resize!=1)) {
		    printf("%s\n", "exessive or missing resize values");
		    usage();
	    }
	    delim_pos=strcspn(dims,"x");
	    tw=atoi(dims);
	    th=atoi(dims+delim_pos+1);
	    if (!th || !tw)
		    usage();
	    /* this scaling policy requires no actions: "fit" */
	    if (!strncmp(dims+strlen(dims)-1,"^",1)) 
		    scale=FIT;
	    else if (!strncmp(dims+strlen(dims)-1,">",1))
		    scale=SHRINK;
	    else
		    scale=ASPECT;
	}

	if (!outfile) {
		printf("%s\n", "no output file specified");
		exit(1);
	} else if ( (format_ext = strrchr(outfile, '.')) ) {
			format_ext++;
	} else if ( (outfile[strlen(outfile)-1] == '-') && 
		    (outfile[strlen(outfile)-2] == ':') ) {
			format_ext=outfile;
			*rindex (format_ext, ':') = 0;
			outfile="/dev/stdout";
	} else {
		printf("%s\n","could not determine target format, quitting");
		exit(1);
	}
	
	if ( (have_broken) && 
		( (!strncasecmp("jpg", format_ext, 3)) ||
		(!strncasecmp("jpe", format_ext, 3)) ) )
	    format_ext=brokenjpeg_fmt;

	/* at last, the converion itself */
	c_exitcode=convert_imlib(outfile, file, tw, th, format_ext, scale,
				    have_orient);
/* 
* FIXME: throughout the whole code unknown error return is -1 or 1;
* with imlib2, however, error codes mean different things. We need
* to choose some common value in the future
*/
	return (c_exitcode);
}


void usage() {
	printf("%s\n"," usage: convlite [options] input-file [options] outfile");
	printf("%s\n","options: [-broken] -sample|-thumbnail|-resize WWWxHHH");
	exit(1);
}

char * strip_framenum(char *filename) {
	if (filename[strlen(filename)-1] == ']') {
		*rindex(filename,'[') = 0;
	}

	return filename;
}