#!/bin/sh

#FIXME:
#FIXME: need a test_command() function to run command and prepare data for
#FIXME: check_integrity function
#FIXME:

check_integrity() {
#usage: check_integrity OUTPUT_FILENAME OK|FAIL EXITCODE STDERR_OUTPUT
#example check_integrity tmp/output.gif OK $? "$STDERR"
    FILE_EXISTS=1
    OUT_FORMAT=""
    
    if [ "$3" = "" ] ; then
	echo "test suite broken"
	exit -1;
    fi
    if [ "$2" != "OK" -a "$2" != "FAIL" ] ; then
	echo "test suite broken"
	exit -1;
    fi

    if [ -f $1 ] ; then
        FILE_EXISTS=0
    fi
    
    EXPECTED_FORMAT=`echo $1 | awk -F"." {'print $NF'}`
    ACTUAL_FORMAT=`file $1 | awk {'print $2'} | tr '[A-Z]' '[a-z]'`
    # jpeg extension sanitization
    if [ "$EXPECTED_FORMAT" = "jpg" -o "$EXPECTED_FORMAT" = "jpe" ] ; then
	EXPECTED_FORMAT="jpeg"
    fi
    # Netpbm extension sanitization
    if [ "$EXPECTED_FORMAT" = "ppm" -o "$EXPECTED_FORMAT" = "ppm" ] ; then
	EXPECTED_FORMAT="netpbm"
    fi

    
# DON'T OVEROPTIMIZE
# It's not just blind copy-and-paste.
# convlite MUST either return 0 AND output no error message AND create a file, - 
# all at a time; or return non-zero AND output an error message and not write
# anything to a file, even when using stdout
    if [ "$2" = "OK" ] ; then
	if [ "$3" != "0" -o "$4" != "" -o "$FILE_EXISTS" != "0" -o "$EXPECTED_FORMAT" != "$ACTUAL_FORMAT" ] ; then
	    echo "failed"
	    exit -1
	else
	    echo "passed"
	    return 0
	fi
    fi
    if [ "$2" = "FAIL" ] ; then
	if [ "$3" = "0" -o "$4" = "" -o "$FILE_EXISTS" = "0" ] ; then
	    echo "failed"
	    exit -1
	else
	    echo "passed"
	    return 0
	fi
    fi
    echo "test suite broken: reached past all test conditions"
    exit -1
}

performance() {
    echo "running performance benchmark"
    echo "-----------------------------"
    echo "resizing gif on uncached fs with convlite"
    echo
    time ./convlite -define registry:temporary-path=test/tmp test/25417.gif -coalesce -auto-orient -thumbnail 200x200 test/tmp/convlite.gif
    echo
    echo "resizing gif on cached fs with convert"
    echo
    time convert -define registry:temporary-path=test/tmp test/25417.gif -coalesce -auto-orient -thumbnail 200x200 test/tmp/convert.gif
    echo
    echo "resizing gif on cached fs with convlite"
    echo
    time ./convlite -define registry:temporary-path=test/tmp test/25417.gif -coalesce -auto-orient -thumbnail 200x200 test/tmp/convlite.gif
    echo
    echo "resizing and converting jpeg to png with convert"
    echo
    time convert -define registry:temporary-path=test/tmp test/ashot.jpg -coalesce -auto-orient -thumbnail 200x200 test/tmp/convert.png
    echo
    echo "resizing and converting jpeg to png with convlite"
    echo
    time ./convlite -define registry:temporary-path=test/tmp test/ashot.jpg -coalesce -auto-orient -thumbnail 200x200 test/tmp/convlite.png
    echo
    echo "resizing and converting jpeg to gif with convert"
    echo
    time convert -define registry:temporary-path=test/tmp test/ashot.jpg -coalesce -auto-orient -thumbnail 200x200 test/tmp/jpgtogif_convert.gif
    echo
    echo "resizing and converting jpeg to gif with convlite"
    echo
    time ./convlite -define registry:temporary-path=test/tmp test/ashot.jpg -coalesce -auto-orient -thumbnail 200x200 test/tmp/jpgtogif_convlite.gif
    sleep 2
    echo
    echo
    echo
}

sanity() {
    echo "running sanity checks"
    echo "---------------------"
    echo -n "convert jpg to png, no resizing... "
    STDERR=$(./convlite test/ashot.jpg test/tmp/jpgtopng.png 2>&1 > /dev/null)
    check_integrity test/tmp/jpgtopng.png OK $? "$STDERR"
    echo -n "convert transparent png to ppm... "
    STDERR=$(./convlite test/transparent.png -resize 100x100 test/tmp/transparentresize.ppm 2>&1 > /dev/null)
    check_integrity test/tmp/transparentresize.ppm OK $? "$STDERR"
    echo -n "resize gif with no extension... "
    STDERR=$(./convlite test/gif.dat -resize 100x100 test/tmp/gifdat.gif 2>&1 > /dev/null)
    check_integrity test/tmp/gifdat.gif OK $? "$STDERR"
    echo -n "resize brokenjpeg with no extension... "
    STDERR=$(./convlite test/jpeg.dat -broken -resize 100x100 test/tmp/jpegdat.jpg 2>&1 > /dev/null)
    check_integrity test/tmp/jpegdat.jpg OK $? "$STDERR"
    echo -n "convert transparent png to ppm, no resizing... "
    STDERR=$(./convlite test/transparent.png test/tmp/transparent.ppm 2>&1 > /dev/null)    
    check_integrity test/tmp/transparent.ppm OK $? "$STDERR"
    echo -n "convert jpg to png, no resizing, through stdout... "
# !!!!!!
#NOTE - no redirection to devnull, we use stdout
# !!!!!!
    STDERR=$(./convlite test/ashot.jpg png:- 2>&1 > test/tmp/jpgtopngstdout.png)
    check_integrity test/tmp/jpgtopngstdout.png OK $? "$STDERR"
    echo -n "convert jpg to gif, no resizing... "
    STDERR=$(./convlite test/ashot.jpg test/tmp/jpgtogif.gif 2>&1 > /dev/null)
    check_integrity test/tmp/jpgtogif.gif OK $? "$STDERR"
# !!!!!!
#NOTE - no redirection to devnull, we use stdout
# !!!!!!
    echo -n "convert jpg to gif, no resizing, through stdout... "
    STDERR=$(./convlite test/ashot.jpg gif:- > test/tmp/jpgtogifstdout.gif)
    check_integrity test/tmp/jpgtogifstdout.gif OK $? "$STDERR"
    echo -n "pass the image, no resizing, through stdout... "
    STDERR=$(./convlite test/ashot.png png:- > test/tmp/pngtopngstdout.png)
    check_integrity test/tmp/pngtopngstdout.png OK $? "$STDERR"
    echo -n "convert false png to jpeg, no resizing... "
    STDERR=$(./convlite test/plaintext.png test/tmp/false.jpg 2>&1 > /dev/null)
    check_integrity test/tmp/false.jpg FAIL $? "$STDERR"
    echo -n "convert false gif to gif, no resizing... "
    STDERR=$(./convlite test/plaintext.gif test/tmp/falsegif.gif 2>&1 > /dev/null)
    check_integrity test/tmp/falsegif.gif FAIL $? "$STDERR"
    echo -n "convert garbage to jpeg, no resizing... "
    STDERR=$( ./convlite test/garbage.dat test/tmp/garbage.jpg 2>&1 > /dev/null)
    check_integrity test/tmp/garbage.jpg FAIL $? "$STDERR"
    echo -n "convert jpeg to jpeg with -broken... "
    STDERR=$( ./convlite test/ashot.jpg -broken test/tmp/brokenjpeg.jpg 2>&1 > /dev/null)
    check_integrity test/tmp/brokenjpeg.jpg OK $? "$STDERR"
# brokenjpeg crash tests. -broken should be ignored if the convertion can be
# performed, otherwise convlite should fail
    echo -n "convert garbage to gif with -broken... "
    STDERR=$( ./convlite test/garbage.dat -broken test/tmp/garbagegif.gif 2>&1 > /dev/null)
    check_integrity test/tmp/garbagegif.gif FAIL $? "$STDERR"
    echo -n "convert gif to gif with -broken... "
    STDERR=$( ./convlite test/25417.gif -broken test/tmp/brokengif.gif 2>&1 > /dev/null)
    check_integrity test/tmp/brokengif.gif OK $? "$STDERR"
    echo -n "convert nonjpeg to jpg with -broken... "
    STDERR=$( ./convlite test/ashot.png -broken test/tmp/nonjpegbroken.jpg 2>&1 > /dev/null)
    check_integrity test/tmp/nonjpegbroken.jpg OK $? "$STDERR"
# end brokenjpeg crash tests
}

# clean up from previous tests, if any
rm -Rf test/tmp
mkdir -p test/tmp

$1
exit 0
