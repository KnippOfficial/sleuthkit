//! \file
//! \author Shujian Yang
//!
//! Header file of class ChunkData

#ifndef CHUNK_DATA_H
#define CHUNK_DATA_H

#include <iostream>
#include <string>
#include <tsk/libtsk.h>
#include <vector>
#include "../../../utils/ReadInt.h"
#include "../../../utils/Uuid.h"

namespace btrForensics {
    //! Chunk item data.
    class ChunkData {
    private:
        uint64_t chunkSize;
        uint64_t objId;

        uint64_t stripeLength; //0x10
        uint64_t type;

        uint32_t optIoAlignment; //0x20
        uint32_t optIoWidth;
        uint32_t minIoSize;

        uint16_t numStripe;
        uint16_t subStripe;

        std::vector<uint64_t> deviceIds;
        std::vector<uint64_t> offsets;
        std::vector<UUID> deviceUUIDs;
    public:
        ChunkData(TSK_ENDIAN_ENUM endian, uint8_t arr[]);

        ~ChunkData() = default; //!< Destructor

        std::string dataInfo() const;

        //! Get offset
        uint64_t getOffset() const { return offsets.at(0); };
        uint64_t getID() const { return deviceIds.at(0); };
        UUID getUUID() const { return deviceUUIDs.at(0); };

        uint64_t getType() const { return type;};
        uint64_t getStripeLength() const { return stripeLength;};
        uint64_t getOffset(int i) const { return offsets.at(i); };
        uint64_t getID(int i) const { return deviceIds.at(i); };

        uint16_t getNumStripe() const { return numStripe;};
        UUID getUUID(int i) const { return deviceUUIDs.at(i); };


        static const int SIZE_OF_CHUNK_DATA = 0x50; //!< Size of chunk item data in bytes.
    };
}

#endif

