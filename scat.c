/* sc.c ::: main program   --*
 * -- version:  A0030      --*
 * -- modified: 2 Jan 2003 --*/

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

#define xerror(str) { realxerror(str,*argv); }

#ifdef DEBUG
#  define debug(str) { fprintf(stderr,"/* %s */\n",str); } 
#else
#  define debug(str) {}
#endif

inline void realxerror(char *str, char* pname)
{
  fprintf(stderr,"%s: %s: %s\n",pname,str,strerror(errno)); 
  exit(EXIT_FAILURE);
}

inline void showUsage(char* progname)
{
  fprintf(stderr,
    "Usage: %s [options] [file [arguments]]\n\n"
    "Options:\n"
    "  -s, --shell=NEWSHELL   change the default shell to NEWSHELL\n"
    "  -h, --help             display this help and exit\n"
    "  -v, --version          output version information and exit\n\n",progname);
}

inline void showVersion(void)
{
  fprintf(stderr,
     "ShellCAT " C_VERSION "\n\n"
     "There is NO warranty. You may redistribute this software\n"
     "under the terms of the GNU General Public License.\n"
     "For more information about these matters, see the file named COPYING.\n\n",
     C_VERSION);
}

inline size_t fprint(FILE* stream, char* str, int len)
{
  return fwrite(str,sizeof(char),len,stream);
}

int main(int argc, char** argv)
{
  static char fFShell[C_MINBUFSIZE] = "sh";
  static int fFVersion = 0;
  static int fFHelp    = 0;
  int a;
  
  while(true)
  {      
    static struct option fOptions[]=
    {
      {"shell",1,0,'s'},
      {"version",0,0,'v'},
      {"help",0,0,'h'},
      {0,0,0,0}
    };

    int fOptIndex = 0;
        
    int c = getopt_long(argc,argv,"vhs:",fOptions,&fOptIndex);
    if (c<0) break;
    if (c==0) c=fOptions[fOptIndex].val;
    switch(c)
    {
      case 'v': 
        fFVersion=1;
        break;
      case 'h':
        fFHelp=1;
        break;
      case 's':
        if (optarg)
        {
          strncpy(fFShell,optarg,sizeof fFShell - 1);
          fFShell[sizeof fFShell]=0;
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
    int i;
    char* fFilename;
    char* fBuffer;
    char* fBufptr;
    char* fBufst;
    FILE* fInput;
    FILE* fOutput;

    fFilename=argv[optind++];
    fInput = fopen(fFilename,"r");
    if (fInput == NULL)
      xerror(fFilename);
    fseek(fInput,0,SEEK_END);
    fSize=ftell(fInput);
    fseek(fInput,0,SEEK_SET);
    fBuffer=(char*)calloc(fSize+2,sizeof(char));
    if (fBuffer==NULL)
      xerror("allocating memory");
    memset(fBuffer,0,fSize+2);
    fread(fBuffer,sizeof(char),fSize,fInput);
    fclose(fInput);
                
#ifndef DEBUG
    fOutput = popen(fFShell,"w");
    if (fOutput == NULL)
      xerror(fFShell);
#else
    fOutput=stdout;
#endif

#define scriptFlush          { fwrite(fBufst,fBufptr-fBufst,sizeof(char),fOutput); fBufst=fBufptr; }
#define scriptFlushI(i)      { fwrite(fBufst,fBufptr-fBufst+i,sizeof(char),fOutput); fBufst=fBufptr; }
#define scriptWrite(str,len) { fprint(fOutput,str,len); }
#define scriptFlushWrite(str,len,add) \
                             { scriptFlush; scriptWrite(str,len); fBufst+=add; }
        
    scriptWrite("set - ",6);
    while(optind<argc)
    {
      char* x=argv[optind++];
      scriptWrite("\"",1);
      while(*x)
      {
        if(*x=='"' || *x=='\\')
          scriptWrite("\\",1);
        scriptWrite(x,1);
        x++;
      }
      scriptWrite("\" ",2);
    }
    scriptWrite("\necho -e \"",10);
    fBufptr = fBuffer;
    fBufst  = fBuffer;
    fCode = false;
      
    for(a=0;a<fSize;(a++),(fBufptr++))
    {
      switch(fBufptr[0])
      {
        case '\\':
          if (!fCode)
            scriptFlushWrite("\\\\",2,1);
          break;
        case '"':
          if (!fCode)
            scriptFlushWrite("\\\"",2,1);
          break;
        case '<':
          if ((!fCode) && fBufptr[1]=='$')
          {
            if (fBufptr[2]=='-')
            {
              scriptFlushWrite("<\\$",3,3);
              fBufptr++; a++;
            }
            else
            {
              scriptFlushWrite("\\c\";\n",5,2);
              fBufptr++; a++;
              fCode=true;
            }
          }
          break;
        case '-':
          if (fCode && fBufptr[1]=='$' && fBufptr[2]=='>')
          {
            scriptFlushWrite("$>",2,3);
            fBufptr++; a++;
          }
          break;
        case '$':
          if (fCode)
          {
            if (fBufptr[1]=='>')
            {
              scriptFlushWrite("\necho -e \"",10,2);
              fBufptr++; a++;
              fCode=false;
            }
          }
          else 
            scriptFlushWrite("\\$",2,1);
          break;
      }
    }
    scriptFlush;      
    if (!fCode) 
      scriptWrite("\\c\"",3);
#ifndef DEBUG
    fclose(fOutput);
#endif
    free(fBuffer);
  }
}
