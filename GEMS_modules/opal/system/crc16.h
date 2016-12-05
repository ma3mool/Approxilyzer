#ifndef __CRC16_H__
#define __CRC16_H__

/*
 * ===============================================================================
 *
 * 	Calculates a checksum for a block of data
 * 		using the CCITT standard CRC-16.
 *
 * ===============================================================================
 */

void CRC16_InitChecksum( unsigned short &crcvalue );
void CRC16_UpdateChecksum( unsigned short &crcvalue, void *data, int length );
void CRC16_FinishChecksum( unsigned short &crcvalue );
unsigned short CRC16_BlockChecksum( void *data, int length );

#endif /* !__CRC16_H__ */

