

#ifndef USB_PCDC_H
#define USB_PCDC_H

#define DATA_LEN                       (512U)
#define NUM_STRING_DESCRIPTOR          (7U)
#define RESET_VALUE                    (0)
#define LINE_CODING_LENGTH             (0x07)

void usb_send(uint8_t *buf, int16_t len);

#endif /* USB_PCDC_H */
