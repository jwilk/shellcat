/* Copyright © 2001, 2002, 2004, 2005, 2008
 * Jakub Wilk <jwilk@jwilk.net>.
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the Free
 * Software Foundation; either version 2 of the License, or (at your option)
 * any later version.
 * 
 * This program is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License for
 * more details.
 */

#include <errno.h>
#include <getopt.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#ifdef LARGEBUFFER
#  define BUFFER_SIZE (1 << 14)
#else
#  define BUFFER_SIZE (1 << 10)
#endif

#ifdef VERSION
#  define C_VERSION VERSION
#else
#  define C_VERSION "(devel)"
#endif

#define xerror(str) { realxerror(str, *argv); }

#ifdef DEBUG
#  define debug(str) { fprintf(stderr, "/* %s */\n", str); } 
#else
#  define debug(str) {}
#endif

#define STDIN_FILENO_DUP 3

inline void realxerror(char *str, char* pname)
{
  fprintf(stderr,"%s: %s: %s\n", pname, str, strerror(errno)); 
  exit(EXIT_FAILURE);
}

inline void show_usage(char* progname)
{
  fprintf(stderr,
    "Usage: %s [options] [file [arguments]]\n\n"
    "Options:\n"
    "  -s, --shell=NEWSHELL   change the default shell to NEWSHELL\n"
    "  -h, --help             display this help and exit\n"
    "  -v, --version          output version information and exit\n\n",
    progname);
}

inline void show_version(void)
{
  fprintf(stderr,
    "ShellCAT " C_VERSION "\n\n"
    "There is NO warranty. You may redistribute this software\n"
    "under the terms of the GNU General Public License.\n"
    "For more information about these matters, see the file named COPYING.\n\n");
}

inline size_t fprint(FILE *stream, char *str, int len)
{
  return fwrite(str, sizeof(char), len, stream);
}

int main(int argc, char **argv)
{
  int a;
  char shell[BUFFER_SIZE];
  bool opt_version = false;
  bool opt_help = false;

  strcpy(shell, "/bin/sh");
  
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
    switch(c)
    {
      case 'v': 
        opt_version = true;
        break;
      case 'h':
        opt_help = true;
        break;
      case 's':
        if (optarg)
        {
          strncpy(shell, optarg, BUFFER_SIZE-1);
          shell[BUFFER_SIZE-1] = 0;
        }
        else
          *shell = '\0';
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
    int filesize;
    char *filename;
    char *buffer, *buftail, *bufhead;
    FILE *instream, *outstream;

    filename = argv[optind++];
    instream = fopen(filename, "r");
    if (instream == NULL)
      xerror(filename);
    if (fseek(instream, 0, SEEK_END) == -1)
      xerror(filename);
    filesize = ftell(instream);
    if (filesize == -1)
      xerror(filename);
    if (fseek(instream, 0, SEEK_SET) == -1)
      xerror(filename);
    buffer = (char*)calloc(filesize+1, sizeof(char));
    // The `+1' is enough. Indeed, we look at (i+1)-th char only if we're sure
    // that i-th char is not '\0'  
    if (buffer == NULL)
      xerror("memory allocation");
    fread(buffer, sizeof(char), filesize, instream);
    if (ferror(instream))
      xerror(filename);
    if (fclose(instream) == EOF)
      xerror(filename);
                
#ifndef DEBUG
    if (dup2(STDIN_FILENO, STDIN_FILENO_DUP) == -1)
      xerror("dup2");
    outstream = popen(shell, "w");
    if (outstream == NULL)
      xerror(shell);
#else
    outstream = stdout;
#endif

#define script_flush \
  do { fprint(outstream, bufhead, buftail-bufhead); bufhead = buftail; } while (false)
#define script_write(str, len) \
  do { fprint(outstream, str, len); } while(false)
#define script_flush_write(str, len, add) \
  do { script_flush; script_write(str, len); bufhead += add; } while (false)
        
    script_write("set - ", 6);
    while (optind < argc) // forward parameters to the script
    {
      char* arg = argv[optind++];
      script_write("\"", 1);
      while (*arg)
      {
        if (*arg == '$' || *arg == '`' || *arg=='"' || *arg=='\\')
          script_write("\\", 1);
        script_write(arg, 1);
        arg++;
      }
      script_write("\" ", 2);
    }
    script_write(
      "\nexec <&3 3<&-"
      "\nprintf '%b' \'", 28);
    buftail = buffer;
    have_code = false;

    a = 0;
    if (buftail[0] == '#' && buftail[1] == '!') // skip the #!... directive
      for ( ; a < filesize; a++, buftail++)
      if (*buftail == '\n')
      {
        buftail++; a++;
        break;
      }
    bufhead = buftail;
    for ( ; a<filesize; a++, buftail++)
    {
      if (!have_code && ((unsigned char)buftail[0]<' ' || buftail[0]=='\'' || buftail[0]=='\177'))
      {
        char oct[6];
        sprintf(oct, "\\%04o", (unsigned int)buftail[0]);
        if (!have_code) script_flush_write(oct, 5, 1);
      }
      else
      switch (buftail[0])
      {
        case '\\':
          if (!have_code)
            script_flush_write("\\\\", 2, 1);
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
            buftail++; a++;
          }
          break;
        case '-':
          if (have_code && buftail[1]=='$' && buftail[2]=='>')
          {
            script_flush_write("$>", 2, 3);
            buftail++; a++;
          }
          break;
        case '$':
          if (have_code && buftail[1] == '>')
          {
            script_flush_write("\nprintf '%b' \'", 14, 2);
            buftail++; a++;
            have_code=false;
          }
          break;
      }
    }
    script_flush;
    if (!have_code) 
      script_write("\'\n", 2);
    script_write("exec <&-\n", 10);
#ifndef DEBUG
    if (fclose(outstream) == EOF)
      xerror(shell);
    close(STDIN_FILENO_DUP);
#endif
    free(buffer);
  }
  return EXIT_SUCCESS;
}

// vim:ts=2 sw=2 et
