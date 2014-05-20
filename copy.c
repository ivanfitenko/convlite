#include <fcntl.h>
#include <unistd.h>
#include <errno.h>

#include <stdio.h>
#include <string.h>

int fd_to, fd_from;
ssize_t nread;
int saved_errno;

int out_error();

int copy_unmodified(const char *src, const char *dest)
{
    char buf[4096];

    fd_from = open(src, O_RDONLY);
    if (fd_from < 0)
        return -1;

    if (strncmp(dest, "/dev/stdout", 11) == 0)
	fd_to = STDOUT_FILENO;
    else
	fd_to = open(dest, O_WRONLY | O_CREAT | O_EXCL, 0644);
    /* okaaay, maybe stdout cannot be < 0 , but who knows what happens next. */
    if (fd_to < 0) {
	fprintf (stderr, "failed to open output stream\n"); 
        return out_error();
    }

    while (nread = read(fd_from, buf, sizeof buf), nread > 0)
    {
        char *out_ptr = buf;
        ssize_t nwritten;

        do {
            nwritten = write(fd_to, out_ptr, nread);
            if (nwritten >= 0)
            {
                nread -= nwritten;
                out_ptr += nwritten;
            }
            else if (errno != EINTR)
            {
                return out_error();
            }
        } while (nread > 0);
    }

    if (nread == 0)
    {
        if (close(fd_to) < 0)
        {
            fd_to = -1;
            return out_error();
        }
        close(fd_from);

        /* Success! */
        return 0;
    }
/* should never reach this point */
    fprintf(stderr, "should've never reached this point in code.c.");
    fprintf(stderr, "PLEASE CONTACT THE AUTORS, WE NEED TO FIX IT\n");
    return out_error();

}

int out_error()
{
    saved_errno = errno;

    close(fd_from);
    if (fd_to >= 0)
        close(fd_to);

    errno = saved_errno;
    return -1;
}
