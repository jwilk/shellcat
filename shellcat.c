/* Copyright © 2001-2017 Jakub Wilk <jwilk@jwilk.net>
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

#include <assert.h>
#include <ctype.h>
#include <errno.h>
#include <getopt.h>
#include <libgen.h>
#include <signal.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>

#ifndef VERSION
#   define VERSION "(devel)"
#endif

static const char *progname = "shellcat";

static char * pipepath = NULL; /* this needs to be global, so that we can
                                  remove the pipe in the SIGCHLD handler */

static void fail(const char *s)
{
    fprintf(stderr,"%s: %s: %s\n", progname, s, strerror(errno));
    exit(EXIT_FAILURE);
}

static void afail(const char *s)
/* async-signal-safe version of fail() */
{
    (void) (
        write(STDERR_FILENO, progname, strlen(progname)) &&
        write(STDERR_FILENO, ": ", 2) &&
        write(STDERR_FILENO, s, strlen(s)) &&
        (errno == 0 || write(STDERR_FILENO, ": unknown error", 15)) &&
        write(STDERR_FILENO, "\n", 1)
    );
    _exit(EXIT_FAILURE);
}

static void show_usage(FILE *fp)
{
    fprintf(fp,
        "Usage: %s [options] FILE [ARGUMENT...]\n\n"
        "Options:\n"
        "  -s, --shell=SHELL   change the default shell to SHELL\n"
        "  -h, --help          display this help and exit\n"
        "  -v, --version       output version information and exit\n\n",
        progname
    );
}

static void show_version(void)
{
    printf("shellcat " VERSION "\n");
}

static char * create_pipe()
{
    int rc;
    char *path;
    const char *tmpdir = getenv("TMPDIR");
    if (tmpdir == NULL)
        tmpdir = "/tmp";
    path = malloc(strlen(tmpdir) + 22);
    if (path == NULL)
        fail("malloc");
    assert(path != NULL);
    sprintf(path, "%s/shellcat.XXXXXX", tmpdir);
    if (mkdtemp(path) == NULL)
        fail("mkdtemp");
    strcat(path, "/pipe");
    rc = mkfifo(path, 0600);
    if (rc != 0)
        fail("mkfifo");
    return path;
}

static void rm_pipe(char *path, void (fail)(const char *))
{
    int rc;
    char *sep;
    rc = unlink(path);
    if (rc != 0)
        fail("unlink");
    sep = strrchr(path, '/');
    assert(sep != NULL);
    assert(sep > path);
    *sep = '\0';
    rc = rmdir(path);
    *path = '\0';
    if (rc != 0)
        fail("rmdir");
}

static void free_pipe(char *path)
{
    rm_pipe(path, fail);
    free(path);
}

static void sigchld_handler(int signal)
{
    (void) signal; /* unused */
    if (pipepath != NULL)
        rm_pipe(pipepath, afail);
    errno = 0;
    afail("shell terminated prematurely");
}

static int reap_child()
{
    int rc;
    if (wait(&rc) == -1)
        fail("wait");
    return rc;
}

static void process_input(FILE *pipe, char **argv)
{
    FILE *input;
    const char *filename = *argv++;
    enum
    {
        STATE_BEGIN,
        STATE_HASH,
        STATE_HASH_BANG,
        STATE_TEXT,
        STATE_TEXT_LT,
        STATE_TEXT_LT_DOLLAR,
        STATE_CODE,
        STATE_CODE_DOLLAR,
        STATE_CODE_MINUS,
        STATE_CODE_MINUS_DOLLAR
    } state;

#define sputs(s) do { fputs(s, pipe); } while (0)
#define sputc(c) do { fputc(c, pipe); } while (0)

    sputs("set -- ");
    for (; *argv != NULL; argv++) /* forward parameters to the script */
    {
        const char* arg = *argv;
        sputs("\'");
        for (; *arg; arg++)
        {
            if (*arg == '\'')
                sputs("'\\'");
            sputc(*arg);
        }
        sputs("\' ");
    }
    sputc('\n');

    input = fopen(filename, "r");
    if (input == NULL)
        fail(filename);

    sputs("printf '%s' \'");
    state = STATE_BEGIN;
    while (!feof(input))
    {
        char buffer[BUFSIZ];
        size_t size;
        const char *bufhead, *buftail;
        bufhead = buftail = buffer;
        size = fread(buffer, 1, sizeof buffer, input);
        if (ferror(input))
            fail(filename);

#define sflush(newstate) \
    do { \
        if (buftail > bufhead && \
            fwrite(bufhead, buftail - bufhead, 1, pipe) != 1) \
                fail("fwrite"); \
        bufhead = buftail + 1; \
        state = newstate; \
    } while (0)

#define srewind(newstate) \
    do { \
        bufhead = buftail; \
        state = newstate; \
        goto rewind; \
    } while (0)

#define sreset(newstate) \
    do { \
        bufhead = buftail + 1; \
        state = newstate; \
    } while (0)

        for (; size > 0U; buftail++, size--)
        {
            const char ch = *buftail;
        rewind:
            switch (state)
            {
                case STATE_BEGIN:
                    if (ch == '#')
                        state = STATE_HASH;
                    else
                        srewind(STATE_TEXT);
                    break;
                case STATE_HASH:
                    if (ch == '!')
                        sreset(STATE_HASH_BANG);
                    else
                    {
                        sputc('#');
                        srewind(STATE_TEXT);
                    }
                    break;
                case STATE_HASH_BANG:
                    if (ch == '\n')
                        sreset(STATE_TEXT);
                    else
                        sreset(STATE_HASH_BANG);
                    break;
                case STATE_TEXT:
                    switch (ch)
                    {
                        case '\'':
                            sflush(STATE_TEXT);
                            sputs("'\\''");
                            break;
                        case '\0':
                            sflush(STATE_TEXT);
                            sputs("'\nprintf '\\000%s' '");
                            break;
                        case '<':
                            sflush(STATE_TEXT_LT);
                            break;
                    }
                    break;
                case STATE_TEXT_LT:
                    switch (ch)
                    {
                        case '$':
                            sreset(STATE_TEXT_LT_DOLLAR);
                            break;
                        default:
                            sputc('<');
                            srewind(STATE_TEXT);
                            break;
                    }
                    break;
                case STATE_TEXT_LT_DOLLAR:
                    switch (ch)
                    {
                        case '-':
                            sputs("<$");
                            sreset(STATE_TEXT);
                            break;
                        default:
                            sputs("'\n");
                            srewind(STATE_CODE);
                            break;
                    }
                    break;
                case STATE_CODE:
                    switch (ch)
                    {
                        case '-':
                            sflush(STATE_CODE_MINUS);
                            break;
                        case '$':
                            sflush(STATE_CODE_DOLLAR);
                            break;
                    }
                    break;
                case STATE_CODE_MINUS:
                    switch (ch)
                    {
                        case '$':
                            sreset(STATE_CODE_MINUS_DOLLAR);
                            break;
                        default:
                            sputc('-');
                            srewind(STATE_CODE);
                            break;
                    }
                    break;
                case STATE_CODE_MINUS_DOLLAR:
                    switch (ch)
                    {
                        case '>':
                            sputs("$>");
                            sreset(STATE_CODE);
                            break;
                        default:
                            sputs("-$");
                            srewind(STATE_CODE);
                            break;
                    }
                    break;
                case STATE_CODE_DOLLAR:
                    switch (ch)
                    {
                        case '>':
                            sputs("\nprintf '%s' '");
                            sreset(STATE_TEXT);
                            break;
                        default:
                            sputc('$');
                            srewind(STATE_CODE);
                    }
                    break;
            }
        }
        sflush(state);
    }

    switch (state)
    {
        case STATE_BEGIN:
        case STATE_HASH:
        case STATE_HASH_BANG:
        case STATE_TEXT:
            sputs("'\n");
            break;
        case STATE_TEXT_LT:
            sputs("<'\n");
            break;
        case STATE_TEXT_LT_DOLLAR:
            sputs("<$'\n");
            break;
        default:
            break;
    }

#undef sputs
#undef sputc
#undef sflush
#undef srewind
#undef sreset

    if (fclose(input) == EOF)
        fail(filename);

}

static bool is_shell_simple(const char *s)
{
    for (; *s; s++)
    {
        switch (*s)
        {
            case '/':
            case '_':
            case '-':
                continue;
        }
        if (isalnum(*s))
            continue;
        return false;
    }
    return true;
}

static const char * build_commandline(const char *shell, const char *arg)
{
    static char buffer[4096]; /* _POSIX_ARG_MAX */
    const char *argptr;
    int isize = snprintf(buffer, sizeof buffer, "%s '", shell);
    if (isize < 0)
        fail("command-line");
    size_t size = (size_t) isize;
    if (size + 2 >= sizeof buffer)
    {
        errno = E2BIG;
        fail("command-line");
    }
    for (argptr = arg; *argptr; argptr++)
    {
        if (size + 6 >= sizeof buffer)
        {
            errno = E2BIG;
            fail("command-line");
        }
        if (*argptr == '\'')
        {
            strcpy(buffer + size, "'\\''");
            size += 4;
        }
        else
        {
            buffer[size] = *argptr;
            size += 1;
        }
    }
    strcpy(buffer + size, "'");
    return buffer;
}

int main(int argc, char **argv)
{
    int rc = EXIT_SUCCESS;
    const char *shell = "/bin/sh";
    bool opt_version = false;
    bool opt_help = false;

    if (setenv("SHELLCAT", argv[0], false) != 0)
        fail("setenv");
    {
        const char *p = strrchr(argv[0], '/');
        if (p)
            progname = p + 1;
        else
            progname = argv[0];
    }

    while (true)
    {
        static struct option options[] =
        {
            { "shell",   1, 0, 's' },
            { "version", 0, 0, 'v' },
            { "help",    0, 0, 'h' },
            { NULL,      0, 0, '\0' }
        };

        int longindex = 0;

        int c = getopt_long(argc, argv, "vhs:", options, &longindex);
        if (c < 0) break;
        if (c == 0) c = options[longindex].val;
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
            case '?':
                return EXIT_FAILURE;
            default:
                break;
        }
    }

    if (opt_version)
        show_version();
    else if (opt_help)
        show_usage(stdout);
    else if (optind >= argc) {
        show_usage(stderr);
        exit(1);
    }
    else
    {
        FILE *pipe;
        pipepath = create_pipe();
        if (signal(SIGCHLD, sigchld_handler) == SIG_ERR)
            fail("signal");
        switch (fork())
        {
            case -1:
                fail("fork");
                break;
            case 0:
                if (is_shell_simple(shell))
                    execlp(shell, shell, pipepath, (char*) NULL);
                else
                {
                    const char * commandline = build_commandline(shell, pipepath);
                    execlp("sh", "sh", "-c", commandline, (char *) NULL);
                }
                fail(shell);
        }
        pipe = fopen(pipepath, "w");
        if (pipe == NULL)
            fail(pipepath);
        if (signal(SIGCHLD, SIG_DFL) == SIG_ERR)
            fail("signal");
        free_pipe(pipepath);
        pipepath = NULL;
        process_input(pipe, argv + optind);
        if (fclose(pipe) == EOF)
            fail(pipepath);
        if (reap_child() != 0)
            rc = EXIT_FAILURE;
    }
    return rc;
}

/* vim:set ts=4 sts=4 sw=4 et: */
