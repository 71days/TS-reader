#ifndef BYTE_PARSER_H_
#define BYTE_PARSER_H_

#include <stdint.h>
#include <assert.h>

using namespace std;

#define BITS_IN_BYTE 8


class byteParser
{
private:
	int8_t *m_poz;
	int8_t m_bitPoz;

	uint32_t m_bitCounter;
	int64_t m_init_poz;

	uint32_t m_bytesParsed;

public: 
	byteParser(int8_t *initPoz);
	template <class T>
	T getBits(int8_t numOfBits);
	void jumpBytes(int16_t numOfBytes);
	void resetBitCounter();
	uint32_t getBitCounter();
	uint32_t getBytesParsed();
};


/* 
This class makes the parsing semi-transparent for us
(we don't need to calculate offsets and masks for each field) 
Notes:
Disavantages: ovearhead
Might implement it as a macro, in the future, so the overhead will be gone,
(the preprocessor will generate the code as we would write the masks by hand),
but as a macro it will be possible to be used only for static structures known at compile time. 
*/
byteParser::byteParser(int8_t *initPoz)
{
	m_poz			= initPoz;
	m_init_poz		= (int64_t)initPoz;
	m_bitPoz		= 0; 
	m_bitCounter	= 0;

}
template <class T>
/* To Do, add get bytes */
T byteParser::getBits(int8_t numOfBits)
{
	T finalVal = 0;
	T val;
	int8_t bitsRead = 0;
	int8_t bitsToRead;
	int8_t remainingBits;

	assert(numOfBits > 0);
	m_bitCounter+= numOfBits;

	remainingBits = numOfBits;

	while (remainingBits != 0)
	{
		bitsToRead = min<int8_t>(remainingBits, 8 - m_bitPoz);

		val = *m_poz & (0xff >> m_bitPoz);
		val = val >> (8 - bitsToRead - m_bitPoz);

		bitsRead += bitsToRead;
		finalVal |= val << (numOfBits - bitsRead);


		remainingBits -= bitsToRead;

		m_bitPoz		+= bitsToRead;
		m_poz			+= m_bitPoz >> 3;
		m_bitPoz %= 8;
	}
	return finalVal;

}

void byteParser::jumpBytes(int16_t numOfBytes)
{
	assert(numOfBytes >= 0);
	assert(m_bitPoz == 0); /* check we are at the begining of the byte */
	m_poz += numOfBytes;

	m_bitCounter += numOfBytes * 8;

}

void byteParser::resetBitCounter()
{
	m_bitCounter = 0;
}

uint32_t byteParser::getBitCounter()
{
	return m_bitCounter;
}

uint32_t byteParser::getBytesParsed()
{
	return ((int64_t)m_poz - m_init_poz);
}


#endif //BYTE_PARSER_H_