/* Copyright © 2001, 2002, 2004, 2005, 2008, 2013
 * Jakub Wilk <jwilk@jwilk.net>.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the “Software”), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED “AS IS”, WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */

#include <errno.h>
#include <getopt.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#ifndef VERSION
#   define VERSION "(devel)"
#endif

#define STDIN_FILENO_DUP 3

static const char *progname = "shellcat";

static void fail(const char *s)
{
    fprintf(stderr,"%s: %s: %s\n", progname, s, strerror(errno));
    exit(EXIT_FAILURE);
}

static void show_usage(const char* progname)
{
    fprintf(stderr,
        "Usage: %s [options] [file [arguments]]\n\n"
        "Options:\n"
        "  -s, --shell=SHELL   change the default shell to SHELL\n"
        "  -h, --help          display this help and exit\n"
        "  -v, --version       output version information and exit\n\n",
        progname
    );
}

static void show_version(void)
{
    fprintf(stderr, "shellcat " VERSION "\n");
}

static void fprint(FILE *stream, const char *str, int len)
{
    if (fwrite(str, len, 1, stream) != 1) {
        fail("fwrite");
    }
}

int main(int argc, char **argv)
{
    int rc = EXIT_SUCCESS;
    const char *shell = "/bin/sh";
    bool opt_version = false;
    bool opt_help = false;

    while (true)
    {
        static struct option options[] =
        {
            { "shell",   1, 0, 's' },
            { "version", 0, 0, 'v' },
            { "help",    0, 0, 'h' },
            { NULL,      0, 0, '\0' }
        };

        int optindex = 0;

        int c = getopt_long(argc, argv, "vhs:", options, &optindex);
        if (c < 0) break;
        if (c == 0) c = options[optindex].val;
        switch (c)
        {
            case 'v':
                opt_version = true;
                break;
            case 'h':
                opt_help = true;
                break;
            case 's':
                if (optarg)
                    shell = optarg;
                break;
            default:
                break;
        }
    }

    if (opt_version)
        show_version();
    else if (opt_help || optind >= argc)
        show_usage(*argv);
    else
    {
        bool have_code;
        long filesize;
        char *filename;
        char *buffer, *buftail, *bufhead;
        FILE *instream, *outstream;

        filename = argv[optind++];
        instream = fopen(filename, "r");
        if (instream == NULL)
            fail(filename);
        if (fseek(instream, 0, SEEK_END) == -1)
            fail(filename);
        filesize = ftell(instream);
        if (filesize == -1)
            fail(filename);
        if (filesize >= SIZE_MAX) {
            errno = EOVERFLOW;
            fail(filename);
        }
        if (fseek(instream, 0, SEEK_SET) == -1)
            fail(filename);
        buffer = (char*)malloc(filesize + 1);
        // The `+1' is enough. Indeed, we look at (i+1)-th char only if we're sure
        // that i-th char is not '\0'
        if (buffer == NULL)
            fail("malloc");
        if (fread(buffer, filesize, 1, instream) != 1) {
            if (!ferror(instream))
                errno = EBUSY;
            fail(filename);
        }
        buffer[filesize] = '\0';
        if (ferror(instream))
            fail(filename);
        if (fclose(instream) == EOF)
            fail(filename);

        if (dup2(STDIN_FILENO, STDIN_FILENO_DUP) == -1)
            fail("dup2");
        outstream = popen(shell, "w");
        if (outstream == NULL)
            fail(shell);

#define script_flush \
    do { fprint(outstream, bufhead, buftail-bufhead); bufhead = buftail; } while (false)
#define script_write(str, len) \
    do { fprint(outstream, str, len); } while(false)
#define script_flush_write(str, len, add) \
    do { script_flush; script_write(str, len); bufhead += add; } while (false)

        script_write("set - ", 6);
        while (optind < argc) // forward parameters to the script
        {
            const char* arg = argv[optind++];
            script_write("\'", 1);
            while (*arg)
            {
                if (*arg == '\'')
                    script_write("'\\'", 3);
                script_write(arg, 1);
                arg++;
            }
            script_write("\' ", 2);
        }
        script_write(
            "\nexec <&3 3<&-"
            "\nprintf '%s' \'", 28
        );
        buftail = buffer;
        have_code = false;

        size_t off = 0;
        if (buftail[0] == '#' && buftail[1] == '!') // skip the shebang
            for ( ; off < filesize; off++, buftail++)
                if (*buftail == '\n')
                {
                    buftail++; off++;
                    break;
                }
        bufhead = buftail;
        for ( ; off < filesize; off++, buftail++)
        {
            switch (buftail[0])
            {
                case '\'':
                    if (!have_code)
                        script_flush_write("'\\''", 4, 1);
                    break;
                case '<':
                    if (!have_code && buftail[1] == '$')
                    {
                        if (buftail[2] == '-')
                            script_flush_write("<\\$", 3, 3);
                        else
                        {
                            script_flush_write("\'\n", 2, 2);
                            have_code = true;
                        }
                        buftail++; off++;
                    }
                    break;
                case '-':
                    if (have_code && buftail[1] == '$' && buftail[2] == '>')
                    {
                        script_flush_write("$>", 2, 3);
                        buftail++; off++;
                    }
                    break;
                case '$':
                    if (have_code && buftail[1] == '>')
                    {
                        script_flush_write("\nprintf '%s' \'", 14, 2);
                        buftail++; off++;
                        have_code = false;
                    }
                    break;
            }
        }
        script_flush;
        if (!have_code)
            script_write("\'\n", 2);
        script_write("exec <&-\n", 9);
        switch (pclose(outstream)) {
            case -1:
                fail("pclose");
            case 0:
                break;
            default:
                rc = EXIT_FAILURE;
        }
        close(STDIN_FILENO_DUP);
        free(buffer);
    }
    return rc;
}

// vim:ts=4 sw=4 et
