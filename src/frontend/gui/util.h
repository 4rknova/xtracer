#ifndef XTGUI_UTIL_H_INCLUDED
#define XTGUI_UTIL_H_INCLUDED

#define MIN(a,b) (a>b?b:a)
#define MAX(a,b) (a>b?a:b)
#define CLAMP(x,a,b) ({if(x<a)x=a;else if(x>b)x=b;})

#endif /* XTGUI_UTIL_H_INCLUDED */
