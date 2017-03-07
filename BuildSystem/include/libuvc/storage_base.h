//
// Created by julee on 16-12-8.
//

#ifndef INSTA360_STORAGE_BASE_H
#define INSTA360_STORAGE_BASE_H
#include "libuvc.h"


int uvcext_read_data(uvc_device_handle_t *devh, int index, uint8_t *data, int len);
int uvcext_write_data(uvc_device_handle_t *devh, int index, uint8_t *data, int len);

uint16_t crc16(uint8_t *data_p, int length);

#endif //INSTA360_STORAGE_BASE_H
