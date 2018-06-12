#ifndef DEFINES_H
#define DEFINES_H

#define PTAG(tag) "CrashSDK." tag

#define SAFE_FREE(p) if(p != NULL) {free(p);p=NULL;}
#define SAFE_DELETE(p) if(p != NULL) {delete p; p = NULL;}

#define OFFSET(type,field)	((char *) &((type *) 0)->field - (char *) 0)

#define ARRAY_SIZE(a)	(sizeof (a) / sizeof ((a)[0]))


#endif
