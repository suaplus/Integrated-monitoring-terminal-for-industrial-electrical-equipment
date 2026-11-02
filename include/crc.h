#ifndef __CRC_H__
#define __CRC_H__

extern void InvertUint8(unsigned char *DesBuf, unsigned char *SrcBuf);
extern void InvertUint16(unsigned short *DesBuf, unsigned short *SrcBuf);
extern unsigned short CRC16_MODBUS(unsigned char *puchMsg, unsigned int usDataLen);

#endif