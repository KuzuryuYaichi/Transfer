#ifndef _CRC_TERMINAL_H_
#define _CRC_TERMINAL_H_

static __u16 crc_ccitt(__u16 crc, __u8 c);
__u16 CRC16_CCITT_XMODEM(__u8 * Data ,__u32 Len);

#endif
