
/**
 * **************************************************************************************************
 * * @Brief    Single byte data inversion        
 * * @Param    
 * *            @DesBuf: destination buffer
 * *            @SrcBuf: source buffer
 * * @RetVal    None
 * * @Note      (MSB)0101_0101 ---> 1010_1010(LSB)
 * **************************************************************************************************
 * */

#include "crc.h"

void InvertUint8(unsigned char *DesBuf, unsigned char *SrcBuf)
{
	int i;
	unsigned char temp = 0;

	for(i = 0; i < 8; i++)
	{
		if(SrcBuf[0] & (1 << i))
		{
			temp |= 1<<(7-i);
		}
	}
	DesBuf[0] = temp;
}

/**
 * **************************************************************************************************
 * * @Brief    double byte data inversion        
 * * @Param    
 * *            @DesBuf: destination buffer
 * *            @SrcBuf: source buffer
 * * @RetVal    None
 * * @Note      (MSB)0101_0101_1010_1010 ---> 0101_0101_1010_1010(LSB)
 * **************************************************************************************************
 * */
void InvertUint16(unsigned short *DesBuf, unsigned short *SrcBuf)  
{  
	int i;  
	unsigned short temp = 0;    

	for(i = 0; i < 16; i++)  
	{  
		if(SrcBuf[0] & (1 << i))
		{          
			temp |= 1<<(15 - i);  
		}
	}  
	DesBuf[0] = temp;  
}


unsigned short CRC16_MODBUS(unsigned char *puchMsg, unsigned int usDataLen)  
{  
	unsigned short wCRCin = 0xFFFF;  
	unsigned short wCPoly = 0x8005;  
	unsigned char wChar = 0;  

	while (usDataLen--)     
	{  
		wChar = *(puchMsg++);  
		InvertUint8(&wChar, &wChar);  
		wCRCin ^= (wChar << 8); 

		int i = 0;
		for( i = 0; i < 8; i++)  
		{  
			if(wCRCin & 0x8000) 
			{
				wCRCin = (wCRCin << 1) ^ wCPoly;  
			}
			else  
			{
				wCRCin = wCRCin << 1; 
			}            
		}  
	}  
	InvertUint16(&wCRCin, &wCRCin);  
	return (wCRCin) ;  
} 
