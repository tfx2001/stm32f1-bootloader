#if !defined(__USB_H)
#define __USB_H

#include "libopencm3/usb/usbd.h"

extern usbd_device *msc_dev;

void usb_init(void);

#endif // __USB_H
