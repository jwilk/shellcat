/* scat.c ::: main program  --*
 * -- version:  A0043       --*
 * -- modified: 25 Aug 2004 --*/

#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <getopt.h>
#include <errno.h>
#include <string.h>

#ifdef LARGEBUFFER
#  define C_MINBUFSIZE 16384
#else
#  define C_MINBUFSIZE 1024
#endif

#ifdef VERSION
#  define C_VERSION VERSION
#else
#  define C_VERSION "(devel)"
#endif

#define bool int
#define true 1
#define false 0

#define xerror(str) { realxerror(str, *argv); }

#ifdef DEBUG
#  define debug(str) { fprintf(stderr, "/* %s */\n", str); } 
#else
#  define debug(str) {}
#endif

inline void realxerror(char *str, char* pname)
{
  fprintf(stderr,"%s: %s: %s\n", pname, str, strerror(errno)); 
  exit(EXIT_FAILURE);
}

inline void showUsage(char* progname)
{
  fprintf(stderr,
    "Usage: %s [options] [file [arguments]]\n\n"
    "Options:\n"
    "  -s, --shell=NEWSHELL   change the default shell to NEWSHELL\n"
    "  -h, --help             display this help and exit\n"
    "  -v, --version          output version information and exit\n\n",
    progname);
}

inline void showVersion(void)
{
  fprintf(stderr,
     "ShellCAT " C_VERSION "\n\n"
     "There is NO warranty. You may redistribute this software\n"
     "under the terms of the GNU General Public License.\n"
     "For more information about these matters, see the file named COPYING.\n\n");
}

inline size_t fprint(FILE* stream, char* str, int len)
{
  return fwrite(str, sizeof(char), len, stream);
}

int main(int argc, char** argv)
{
  int a;
  char fFShell[C_MINBUFSIZE];
  bool fFVersion = false;
  bool fFHelp = false;

  strncpy(fFShell, "/bin/sh", C_MINBUFSIZE-1);
  fFShell[C_MINBUFSIZE-1] = 0;
  
  while(true)
  {      
    static struct option fOptions[] =
    {
      {"shell",   1, 0, 's'},
      {"version", 0, 0, 'v'},
      {"help",    0, 0, 'h'},
      {0, 0, 0, 0}
    };

    int fOptIndex = 0;
        
    int c = getopt_long(argc, argv, "vhs:", fOptions, &fOptIndex);
    if (c < 0) break;
    if (c == 0) c=fOptions[fOptIndex].val;
    switch(c)
    {
      case 'v': 
        fFVersion = true;
        break;
      case 'h':
        fFHelp = true;
        break;
      case 's':
        if (optarg)
        {
          strncpy(fFShell, optarg, C_MINBUFSIZE-1);
          fFShell[C_MINBUFSIZE-1] = 0;
        }
        else
          *fFShell=0;
      default:
        break;
    }
  }

  if (fFVersion)
    showVersion();
  else if (fFHelp || optind>=argc)
    showUsage(*argv);
  else
  {
    bool fCode;
    int fSize;
    char *fFilename;
    char *fBuffer, *fBufptr, *fBufst;
    FILE *fInput, *fOutput;

    fFilename = argv[optind++];
    fInput = fopen(fFilename, "r");
    if (fInput == NULL)
      xerror(fFilename);
    fseek(fInput, 0, SEEK_END);
    fSize = ftell(fInput);
    fseek(fInput, 0, SEEK_SET);
    fBuffer = (char*)calloc(fSize+1, sizeof(char));
    // The `+1' is enough. Indeed, we look at (i+1)-th char only if we're sure
    // that i-th char is not '\0'  
    if (fBuffer == NULL)
      xerror("allocating memory");
    fread(fBuffer, sizeof(char), fSize, fInput);
    fclose(fInput);
                
#ifndef DEBUG
    fOutput = popen(fFShell, "w");
    if (fOutput == NULL)
      xerror(fFShell);
#else
    fOutput = stdout;
#endif

#define scriptFlush \
  do { fprint(fOutput, fBufst, fBufptr-fBufst); fBufst=fBufptr; } while (false)
#define scriptFlushI(i) \
  do { fprint(fOutput, fBufst, fBufptr-fBufst+i); fBufst=fBufptr; } while (false)
#define scriptWrite(str, len) \
  do { fprint(fOutput, str, len); } while(false)
#define scriptFlushWrite(str, len, add) \
  do { scriptFlush; scriptWrite(str, len); fBufst+=add; } while (false)
        
    scriptWrite("set - ", 6);
    while (optind < argc) // forward parameters to the script
    {
      char* x=argv[optind++];
      scriptWrite("\"", 1);
      while(*x)
      {
        if (*x=='"' || *x=='\\')
          scriptWrite("\\", 1);
        scriptWrite(x, 1);
        x++;
      }
      scriptWrite("\" ", 2);
    }
    scriptWrite("\nprintf '%b' \'", 14);
    fBufptr = fBuffer;
    fCode = false;

    a = 0;
    if (fBufptr[0]=='#' && fBufptr[1]=='!') // ignore the #!... directive
      for (; a<fSize; a++, fBufptr++)
        if (*fBufptr=='\n')
        {
          fBufptr++; a++;
          break;
        }
    fBufst = fBufptr;
    for(; a<fSize; a++, fBufptr++)
    {
      if (!fCode && ((unsigned char)fBufptr[0]<' ' || fBufptr[0]=='\'' || fBufptr[0]=='\177'))
      {
        char oct[6];
        sprintf(oct, "\\%04o", (unsigned int)fBufptr[0]);
        if (!fCode) scriptFlushWrite(oct, 5, 1);
      }
      else
      switch(fBufptr[0])
      {
        case '\\':
          if (!fCode)
            scriptFlushWrite("\\\\", 2, 1);
          break;
        case '<':
          if (!fCode && fBufptr[1] == '$')
          {
            if (fBufptr[2] == '-')
              scriptFlushWrite("<\\$", 3, 3);
            else
            {
              scriptFlushWrite("\'\n", 2, 2);
              fCode = true;
            }
            fBufptr++; a++;
          }
          break;
        case '-':
          if (fCode && fBufptr[1]=='$' && fBufptr[2]=='>')
          {
            scriptFlushWrite("$>", 2, 3);
            fBufptr++; a++;
          }
          break;
        case '$':
          if (fCode && fBufptr[1] == '>')
          {
            scriptFlushWrite("\nprintf '%b' \'", 14, 2);
            fBufptr++; a++;
            fCode=false;
          }
          break;
      }
    }
    scriptFlush;
    if (!fCode) 
      scriptWrite("\'\n", 2);
#ifndef DEBUG
    fclose(fOutput);
#endif
    free(fBuffer);
  }
  return EXIT_SUCCESS;
}
