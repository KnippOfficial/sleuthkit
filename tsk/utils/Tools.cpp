/*
 * Tools.cpp
 * The Sleuth Kit
 *
 * This software is distributed under the Common Public License 1.0
 *
 */

/**
 * \file Tools.cpp
 * Includes some tools for output and compression
 */

#include "Tools.h"
#include <ctime>

using namespace std;

const string timestampToDateString(uint64_t timestamp){
    time_t t = timestamp;
    string result = std::ctime(&t);
    result.erase(result.find('\n', 0), 1);

    return result;

}

void fletcher_4_native(const uint8_t *buf, uint64_t size, uint64_t* output)
{
    const uint32_t *ip = (uint32_t *) buf;
    const uint32_t *ipend = ip + (size / sizeof (uint32_t));
    uint64_t a, b, c, d;

    for (a = b = c = d = 0; ip < ipend; ip++) {
        a += ip[0];
        b += a;
        c += b;
        d += c;
    }

    *output = a;
    *(output + 0x01) = b;
    *(output + 0x02) = c;
    *(output + 0x03) = d;

}

int roundUp(int numToRound, int multiple)
{
    if (multiple == 0)
        return numToRound;

    int remainder = numToRound % multiple;
    if (remainder == 0)
        return numToRound;

    return numToRound + multiple - remainder;
}