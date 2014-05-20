#include <stdio.h>
#include <libexif/exif-loader.h>

int exif_orientation(char *fname) {
    ExifData *data; 
    data = exif_data_new_from_file(fname);
    if (!data) {
// this fprintf should go into verbose section
//	fprintf (stderr, "unable to retreive exif data\n");
	/* 
	* The way exif data is currently used suggests that 
	* it is safe to proceed with no data, so just return
	* 'no orientation' without an error code
	*/
	return 0;
    }
    ExifEntry *entry; 
    ExifByteOrder byte_order; 
    byte_order= exif_data_get_byte_order(data);

    /* get orientation*/
    if ((entry= exif_content_get_entry( data->ifd[EXIF_IFD_0], 
					EXIF_TAG_ORIENTATION)))
	return (int)exif_get_short(entry->data,byte_order);
    else 
	fprintf (stderr, "exif orientation missing\n");
    exif_data_unref(data);
//    exif_data_free(data);
    /* safe to proceed regardless of errors  so return 'no orientation' */
    return 0;
}
