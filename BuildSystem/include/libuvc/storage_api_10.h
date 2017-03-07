//
// Created by julee on 16-12-8.
//

#ifndef INSTA360_STORAGE_API_10_H
#define INSTA360_STORAGE_API_10_H
#ifdef __cplusplus
extern "C" {
#endif

#include "libuvc/libuvc.h"

int uvcext_read_offset_10(uvc_device_handle_t *devh, char **offset, int *len);

int uvcext_read_uuid_10(uvc_device_handle_t *devh, char **uuid, int *len);

int uvcext_read_serialData_10(uvc_device_handle_t *devh, char **serial, int *len);

int uvcext_read_activation_info_10(uvc_device_handle_t *devh, char **data, int *len);

int uvcext_read_app_data_10(uvc_device_handle_t *devh, char **data, int *len);


int uvcext_write_offset_10(uvc_device_handle_t *devh, char *offset, int len);

int uvcext_write_uuid_10(uvc_device_handle_t *devh, char *uuid, int len);

int uvcext_write_serialData_10(uvc_device_handle_t *devh, char *serialNo, int len);

int uvcext_write_activation_info_10(uvc_device_handle_t *devh, char *data, int len);

int uvcext_write_app_data_10(uvc_device_handle_t *devh, char *data, int len);

int uvcext_ex_write_10(uvc_device_handle_t *devh, int index, uint32_t tag, char *data, int len);

#ifdef __cplusplus
}
#endif
#endif //INSTA360_STORAGE_API_10_H
