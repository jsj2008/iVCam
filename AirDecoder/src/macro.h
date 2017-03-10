
#ifndef PRD_MACRO_H
#define PRD_MACRO_H

/* function */
#define CC_MAX(a,b) (((a) > (b)) ? (a) : (b))
#define CC_MIN(a,b) (((a) < (b)) ? (a) : (b))

#define CC_DELETE_MIN(p) \
if (p) \
{\
	delete p;\
}

#define CC_DELETE(p) \
if (p) \
{\
	delete p;\
	p = NULL;\
}

#define CC_DELETE_ARRAY(p) \
if (p) \
{\
	delete[] p;\
	p = NULL;\
}

#define CC_FREE(p) \
if (p) \
{\
	free(p);\
	p = NULL;\
}

#if (defined _WIN32 || defined _WIN64)
#define CC_STRCMP(src, dst) _stricmp(src, dst)
#define CC_SPRINTF _snprintf
#else
#define CC_STRCMP(src, dst) strcmp(src, dst)
#define CC_SPRINTF snprintf
#endif


#endif

