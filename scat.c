/* sc.c ::: main program    --*
 * -- version:  A0026       --*
 * -- modified: 25 Aug 2001 --*/

#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <getopt.h>
#include <errno.h>
#include <string.h>

#include "lang.inc.c"

#define cMINIBUFSIZE 1024
#define fprint(f,s,l) fwrite(s,sizeof(char),l,f)
#define xerror(s) { fprintf(stderr,"%s: %s: %s\n",argv[0],s,strerror(errno)); _exit(errno); }

char* strVer = "0.02.00";

void showUsage(char* progname)
{
    fprintf(stderr,"%s:",lstrUsage);
    fprintf(stderr," %s [%s] [%s [%s]]\n\n",progname,lstrUsageOptions,lstrUsageFilename,lstrUsageArguments);
    fprintf(stderr,"%s:\n",lstrOptions);
    fprintf(stderr,"  -s, --shell=NEWSHELL   %s\n",lstrUsageOptShell);
    fprintf(stderr,"  -h, --help             %s\n",lstrUsageOptHelp);
    fprintf(stderr,"  -v, --version          %s\n",lstrUsageOptVersion);
    fprint(stderr,"\n",1);
}

void showVersion(void)
{
  fprintf(stderr,"%s %s\n\n%s\n",lstrShellCat,strVer,lstrWarranty);
}

int main(int argc, char** argv)
{
    static char fFShell[cMINIBUFSIZE] = "sh";
    static int fFVersion = 0;
    static int fFHelp    = 0;
  
    while(1)
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
        int fCode;
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
            xerror(lstrErrMemAlloc)
        memset(fBuffer,0,fSize+2);
        fread(fBuffer,sizeof(char),fSize,fInput);
        fclose(fInput);
                
        fOutput = popen(fFShell,"w");
        if (fOutput == NULL)
            xerror(fFShell);
            
        fprint(fOutput,"set - ",6);
        while(optind<argc)
        {
            char* x=argv[optind++];
            fprint(fOutput,"\"",1);
            while(*x)
            {
                if(*x=='"' || *x=='\\')
                    fprint(fOutput,"\\",1);
                fprint(fOutput,x,1);
                x++;
            }
            fprint(fOutput,"\" ",2);
        }
        fprint(fOutput,"\n",1);
        fprint(fOutput,"echo -e \"",9);
        fBufptr = fBuffer;
        fBufst  = fBuffer;
        fCode = 0;
        int a;

#define scriptFlush          { fwrite(fBufst,fBufptr-fBufst,sizeof(char),fOutput); fBufst=fBufptr; }
#define scriptFlushI(i)      { fwrite(fBufst,fBufptr-fBufst+i,sizeof(char),fOutput); fBufst=fBufptr; }
#define scriptWrite(str,len) { fprint(fOutput,str,len); }
      
        for(a=0;a<fSize;(a++),(fBufptr++))
        {
            switch(*fBufptr)
    	    {
    		case '\\':
        	    if (!fCode)
        	    {
            		scriptFlush;
            		scriptWrite("\\\\",2);
            		fBufst++;
        	    }
        	    break;
    		case '"':
        	    if (!fCode)
        	    {
            		scriptFlush; 
            		scriptWrite("\\\"",2);
            		fBufst++;
        	    }
        	    break;
    		case '<':
        	    if (!fCode && *(fBufptr+1)=='$')
        	    {
                    if (*(fBufptr+2)=='-')
                    {
                        scriptFlush;
                        scriptWrite("<\\$",3);
                        fBufst+=3;
                        fBufptr++; a++;
                    }
                    else
                    {
                	    scriptFlush;
                	    scriptWrite("\\c\";\n",5);
            		    fBufst+=2;
                	    fBufptr++; a++;
                	    fCode=1;
                    }
            	}
        	    break;
            case '-':
                if (fCode && *(fBufptr+1)=='$' && *(fBufptr+2)=='>')
                {
                    scriptFlush;
                    scriptWrite("$>",2);
                    fBufst+=3;
                    fBufptr++; a++;
                }
                break;
    		case '$':
        	    if (fCode)
        	    {
			        if (*(fBufptr+1)=='>')
			        {
              	        scriptFlush;
           		        scriptWrite("\necho -e \"",10);
           		        fBufst+=2;
               	        fBufptr++; a++;
               	        fCode=0;
			        }
            	}
        	    else 
        	    {
            		scriptFlush;
            		scriptWrite("\\$",2);
            		fBufst++;
        	    }
        	    break;
    	    }
        }
        scriptFlush;      
        if (!fCode) scriptWrite("\\c\"",3);
      
        fclose(fOutput);
        free(fBuffer);
    }
}

