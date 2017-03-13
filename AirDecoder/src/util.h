
#ifndef _INSUTIL_UTIL_H_
#define _INSUTIL_UTIL_H_

#include <string>
#include "common.h"

//macro func

#define CM_MAX(a,b) (((a) > (b)) ? (a) : (b))
#define CM_MIN(a,b) (((a) < (b)) ? (a) : (b))

#define CM_DELETE(p) \
if (p) \
{\
delete p;\
p = nullptr;\
}

#define CM_DELETE_ARRAY(p) \
if (p) \
{\
delete[] p;\
p = nullptr;\
}

#define CM_FREE(p) \
if (p) \
{\
free(p);\
p = nullptr;\
}

//function

#define FFERR2STR(err) (fferr2str(ret).c_str())

std::string fferr2str(int err);

void ffRegAll();
void ffRegAllCodec();
void ffNetworkInit();

int Convert2FFPixFmt(unsigned char pix_fmt);

#define DEC_PRI_DATA_TYPE_NONE     0
#define DEC_PRI_DATA_TYPE_FRAME    1
#define DEC_PRI_DATA_TYPE_PIC      2
#define DEC_PRI_DATA_TYPE_ARRY     3

//class&&struct


#endif
