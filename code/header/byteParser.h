#ifndef BYTE_PARSER_H_
#define BYTE_PARSER_H_

#include <stdint.h>

class byteParser
{
private:
	int8_t *m_poz;
	int8_t m_bitPoz;
	uint32_t m_bytesParsed;
	uint32_t m_bitCounter;
public: 
	byteParser(int8_t *initPoz);
	template <class T>
	T getBits(int8_t numOfBits);
	void jumpBytes(int16_t numOfBytes);
	void resetBitCounter();
	uint32_t getBitCounter();
	uint32_t getBytesParsed();
};

#endif //BYTE_PARSER_H_