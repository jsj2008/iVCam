//
// Created by julee on 16-9-10.
//

#ifndef INSTA360_LIBUVC_CTRL_H
#define INSTA360_LIBUVC_CTRL_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>
#include <stdbool.h>
#include "libuvc.h"

int uvcext_query_ctrl(uvc_device_handle_t *dev,
                      uint8_t query,
                      uint8_t cs,   // Processing unit control selector (A.9.5)
                      uint8_t unit,
                      uint8_t intfnum,
                      void *data,
                      uint16_t size);

////////////////////////////////////////////////////////////////////////////////////////////////
/* processing unit manual control api */

int uvcext_get_brightness(uvc_device_handle_t *dev, enum uvc_req_code req);

int uvcext_set_brightness(uvc_device_handle_t *dev, int brightness);

int uvcext_get_contrast(uvc_device_handle_t *dev, enum uvc_req_code req);

int uvcext_set_contrast(uvc_device_handle_t *dev, int contrast);

int uvcext_get_hue(uvc_device_handle_t *dev, enum uvc_req_code req);

int uvcext_set_hue(uvc_device_handle_t *dev, int hue);

int uvcext_get_saturation(uvc_device_handle_t *dev, enum uvc_req_code req);

int uvcext_set_saturation(uvc_device_handle_t *dev, int saturation);

int uvcext_get_gain(uvc_device_handle_t *dev, enum uvc_req_code req);

int uvcext_set_gain(uvc_device_handle_t *dev, int gain);

int uvcext_get_sharpness(uvc_device_handle_t *dev, enum uvc_req_code req);

int uvcext_set_sharpness(uvc_device_handle_t *dev, int sharpness);

int uvcext_get_white_balance_temperature(uvc_device_handle_t *dev, enum uvc_req_code req);

int uvcext_set_white_balance_temperature(uvc_device_handle_t *dev, int wbt);


/* processing unit auto control api */

int uvcext_set_contrast_auto(uvc_device_handle_t *dev, bool contrast_auto);

int uvcext_set_hue_auto(uvc_device_handle_t *dev, bool hue_auto);

int uvcext_set_white_balance_temperature_auto(uvc_device_handle_t *dev, bool wbt_auto);


int uvcext_set_power_line_frequency(uvc_device_handle_t *dev, int power_line_frequency);

////////////////////////////////////////////////////////////////////////////////////////////////
/* terminal control api */

int uvcext_set_expose_mode(uvc_device_handle_t *dev, bool expose_auto);

int uvcext_set_expose_time_absolute(uvc_device_handle_t *dev, int expose_time);


////////////////////////////////////////////////////////////////////////////////////////////////
/* extension unit api */

int uvcext_extension_ctrl(uvc_device_handle_t *devh,
                          uint8_t bRequest,
                          uint8_t cs,   // Control selector: Processing unit control selector (A.9.5)
                          uint8_t extension_unit_id,
                          void *data,
                          uint16_t size);


////////////////////////////////////////////////////////////////////////////////////////////////
/* api defined by vendor  */
int uvcext_set_stream_bitrate_xu(uvc_device_handle_t *devh, int bitrate);
int uvcext_set_stream_carry_timestamp(uvc_device_handle_t *devh, bool carry);
int64_t uvcext_get_camera_time(uvc_device_handle_t *devh);

#define LED_COLOR_RED  39
#define LED_COLOR_GREEN 40
#define LED_COLOR_BLUE 50

int uvcext_led_xu(uvc_device_handle_t *devh,
    int on,
    int led_color,
    int enable_blink, int blink_period_ms);

int uvcext_read_version(uvc_device_handle_t *devh, int64_t *version_value, char release_name[32]);
int uvcext_read_builddate(uvc_device_handle_t *devh, char build_date[8]);

#ifdef __cplusplus
}
#endif

#endif //INSTA360_LIBUVC_CTRL_H
