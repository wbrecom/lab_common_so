/**
 * woo framework 
 */
#ifndef __woo_crc_H_
#define __woo_crc_H_

namespace woo {

uint32_t chksum_crc32 (unsigned char *block, unsigned int length, unsigned long crc = 0xFFFFFFFF);

}

#endif
