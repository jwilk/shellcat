/* lang.c ::: language choice --*
 * -- version:  A0002         --*
 * -- modified: 23 Aug 2001   --*/
 
#define SC_LANG_PL
 
#ifdef SC_LANG_EN
# include "lang/english.inc.c"
#endif
#ifdef SC_LANG_PL
# include "lang/polish.inc.c"
#endif
