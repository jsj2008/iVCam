//
// Created by julee on 16-12-8.
//

#ifndef INSTA360_STORAGE_API_H
#define INSTA360_STORAGE_API_H
#ifdef __cplusplus
extern "C" {
#endif

#include "libuvc/libuvc.h"

int uvcext_read_data_layout_version(uvc_device_handle_t *devh, uint16_t *major, uint16_t *minor);
int uvcext_write_data_layout_version(uvc_device_handle_t *devh, uint16_t major, uint16_t minor);

int uvcext_read_offset(uvc_device_handle_t *devh, uint16_t major, char **offset, int *len);
int uvcext_read_uuid(uvc_device_handle_t *devh, uint16_t major, char **uuid, int *len);
int uvcext_read_serialData(uvc_device_handle_t *devh, uint16_t major, char **serial, int *len);
int uvcext_read_activation_info(uvc_device_handle_t *devh, uint16_t major, char **data, int *len);
int uvcext_read_app_data(uvc_device_handle_t *devh, uint16_t major, char **data, int *len);

int uvcext_write_offset(uvc_device_handle_t *devh, uint16_t major, char *offset, int len);
int uvcext_write_uuid(uvc_device_handle_t *devh, uint16_t major, char *uuid, int len);
int uvcext_write_serialData(uvc_device_handle_t *devh, uint16_t major, char *serialNo, int len);
int uvcext_write_activation_info(uvc_device_handle_t *devh, uint16_t major, char *data, int len);
int uvcext_write_app_data(uvc_device_handle_t *devh, uint16_t major, char *data, int len);

int uvcext_ex_write(uvc_device_handle_t *devh, uint16_t major, int index, uint32_t tag, char *data, int len);

#ifdef __cplusplus
}
#endif

#endif //INSTA360_STORAGE_API_H
