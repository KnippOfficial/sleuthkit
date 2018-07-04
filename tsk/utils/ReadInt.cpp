/*The MIT License

Copyright (c) 2016 Shujian Yang

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in
all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
THE SOFTWARE.*/

#include "ReadInt.h"

/**
  * Read a 16-bit long number from byte array.
  *
  * \param endian The endianess of the array.
  * \param arr The array containg the bytes.
  *
  */
uint16_t read16Bit(TSK_ENDIAN_ENUM endian, const uint8_t *arr)
{
    uint16_t num(0);

    if(endian == TSK_LIT_ENDIAN) {
        num += (uint16_t)*(arr + 1);
        num = num << 8;
        num += (uint16_t)*arr;
    }
    else if(endian == TSK_BIG_ENDIAN) {
        num += (uint16_t)*arr;
        num = num << 8;
        num += (uint16_t)*(arr + 1);
    }

    return num;
}

/**
  * Read a 32-bit long number from byte array.
  *
  * \param endian The endianess of the array.
  * \param arr The array containg the bytes.
  *
  */
uint32_t read32Bit(TSK_ENDIAN_ENUM endian, const uint8_t *arr)
{
    uint32_t num(0);

    if(endian == TSK_LIT_ENDIAN) {
        for(int i=3; i>=0; i--){
            num <<= 8;
            num += (uint32_t)*(arr + i);
        }
    }
    else if(endian == TSK_BIG_ENDIAN) {
        for(int i=0; i<=3; i++){
            num <<= 8;
            num += (uint32_t)*(arr + i);
        }
    }

    return num;
}

/**
  * Read a 64-bit long number from byte array.
  *
  * \param endian The endianess of the array.
  * \param arr The array containg the bytes.
  *
  */
uint64_t read64Bit(TSK_ENDIAN_ENUM endian, const uint8_t *arr)
{
    uint64_t num(0);

    if(endian == TSK_LIT_ENDIAN) {
        for(int i=7; i>=0; i--){
            num <<= 8;
            num += (uint64_t)*(arr + i);
        }
    }
    else if(endian == TSK_BIG_ENDIAN) {
        for(int i=0; i<=7; i++){
            num <<= 8;
            num += (uint64_t)*(arr + i);
        }
    }

    return num;
}
