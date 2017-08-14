/*
 * Blkptr.cpp
 * The Sleuth Kit
 *
 * This software is distributed under the Common Public License 1.0
 *
 */

/**
 * \file Blkptr.cpp
 * Describes the Blkptr object in ZFS
 */

#include "Blkptr.h"
#include "pool/ZFS_POOL.h"

Blkptr::Blkptr(TSK_ENDIAN_ENUM endian, uint8_t *data, ZFS_POOL *pool)
        : blk_lsize(0), blk_psize(0), blk_compression(0), blk_checksum_type(0), embedded(0), endian(endian),
          pool(pool), ownData() {
    //TODO: store number of dvs (compare to 0:0?)
    dataAvailable = false;
    blk_dva[0].vdev = read32Bit(endian, data + 0x04);
    blk_dva[0].offset = read64Bit(endian, data + 0x08) << 9;
    blk_dva[1].vdev = read32Bit(endian, data + 0x14);
    blk_dva[1].offset = read64Bit(endian, data + 0x18) << 9;
    blk_dva[2].vdev = read32Bit(endian, data + 0x24);
    blk_dva[2].offset = read64Bit(endian, data + 0x28) << 9;
    blk_lsize = (uint64_t) (read16Bit(endian, data + 0x30) + 1) * 512;
    blk_psize = (uint64_t) (read16Bit(endian, data + 0x32) + 1) * 512;
    blk_asize = (uint64_t) (read64Bit(endian, data + 0x00));
    blk_asize = blk_asize & 0xffffff;
    blk_asize = (blk_asize + 1) * 512;
    blk_checksum[0] = read64Bit(endian, data + 0x60);
    blk_checksum[1] = read64Bit(endian, data + 0x68);
    blk_checksum[2] = read64Bit(endian, data + 0x70);
    blk_checksum[3] = read64Bit(endian, data + 0x78);
    blk_compression = data[52];
    blk_compression &= ~(1 << 7);
    blk_checksum_type = (uint8_t) data[53];

    embedded = (uint8_t) ((data[52] >> 7) & (uint8_t) 1);

    if (embedded) {
        ownData.insert(ownData.end(), data, data + 128);
        dataAvailable = true;
    } else {
       //check if block pointer points to available top-level vdev
        for(int i = 0; i < 3; i++){
            if(pool->getVdevByID(blk_dva[i].vdev) != nullptr){
                dataAvailable = true;
                break;
            }
        }     
    }

    if(dataAvailable || (blk_checksum[0] == 0 && blk_checksum[1] == 0 && blk_checksum[2] == 0 && blk_checksum[3] == 0)){ 
        if (!checkChecksum()) {
            throw (0);
        }
    }
}

bool Blkptr::checkChecksum() {
    if (embedded)
        return true;

    //TODO: Add other checksum types
    if (blk_checksum_type == 7) {
        std::vector<char> dataVector;
        getData(dataVector, 1, false);
        blk_calculated_checksum[0] = 0;
        blk_calculated_checksum[1] = 0;
        blk_calculated_checksum[2] = 0;
        blk_calculated_checksum[3] = 0;
        fletcher_4_native((uint8_t *) &dataVector[0], dataVector.size(), this->blk_calculated_checksum);
    } else {
        return false;
    }

    for (int i = 0; i < 4; ++i) {
        if (blk_checksum[i] != blk_calculated_checksum[i])
            return false;
    }

    return true;
}

//if level == 1: don't go for indirect blocks
void Blkptr::getData(std::vector<char> &dataVector, int level, bool decompress) {
    //TODO: Support for not compressed embedded blockpointers!!!
    if (embedded) {
        std::vector<char> payload;
        uint32_t payloadLength = read32Bit(TSK_BIG_ENDIAN, (uint8_t *) &this->ownData[0]) + 4;
        payload.insert(payload.end(), &this->ownData[0], &this->ownData[0] + 48);
        payload.insert(payload.end(), &this->ownData[0] + 56, &this->ownData[0] + 80);
        payload.insert(payload.end(), &this->ownData[0] + 88, &this->ownData[0] + 128);
        payload.resize(payloadLength);

        //TODO: Embedded decompressed always decompress logical size of 512 bytes?
        if ((payloadLength != 512)) {
            vector<char> decompressedData(512);
            LZ4_decompress_safe(&payload[0] + 4, decompressedData.data(), payloadLength - 4, 512);
            dataVector.insert(dataVector.end(), decompressedData.data(), decompressedData.data() + 512);
        } else {
            dataVector.insert(dataVector.end(), payload.data(), payload.data() + 512);
        }

    } else {
        vector<char> diskData;
        pool->readData(this, diskData, decompress);

        if (level == 1) {   //direct block pointer
            if (decompress) {
                dataVector.insert(dataVector.end(), diskData.data(), diskData.data() + this->blk_lsize);
            } else {
                dataVector.insert(dataVector.end(), diskData.data(), diskData.data() + this->blk_psize);
            }
        } else {  //indirect block pointer
            Blkptr *temp = nullptr;
            for (int i = 0; i < (this->blk_lsize / 128); i++) {
                try {
                    temp = new Blkptr(this->endian, (uint8_t *) diskData.data() + 128 * i, this->pool);
                    temp->getData(dataVector, level - 1);
                    delete temp;
                }
                catch (...) {
                    continue;
                }
            }
        }
    }
}

stringstream Blkptr::level0Blocks(int level) {
    std::stringstream stream;
    vector<char> diskData;
    if (level == 1) {
        if(!this->dataAvailable){
            stream << "[-]" << "\t";
        } else {
            stream << "[*]" << "\t";
        }

        for(int i = 0; i < 3; i++){
            stream << setw(2) << right << this->blk_dva[i].vdev << " : " << hex << setw(10) << left << this->blk_dva[i].offset << "\t|\t";
        }
        stream << this->blk_lsize
               << " / " << this->blk_psize << dec << endl;

    } else {
        pool->readData(this, diskData);
        for (int i = 0; i < (this->blk_lsize / 128); i++) {
            try {
                Blkptr temp(this->endian, (uint8_t *) diskData.data() + 128 * i, this->pool);
                stream << temp.level0Blocks(level - 1).str();
            }
            catch (...) {
                continue;
            }
        }
    }
    return stream;
}

ostream &operator<<(ostream &os, const Blkptr &blkptr) {
    if (blkptr.embedded)
        os << "Embedded Blockpointer" << endl;
    else {
        os << "DVA[0]:\t" << hex << blkptr.getDVA(0).vdev << " : " << blkptr.getDVA(0).offset << "\tDVA[1]:\t"
           << blkptr.getDVA(1).vdev << " : " << blkptr.getDVA(1).offset
           << "\tDVA[2]:\t" << blkptr.getDVA(2).vdev << " : " << blkptr.getDVA(2).offset << dec << endl;
        os << "Logical Size:\t" << blkptr.getLsize() << " bytes\t Physical Size:\t" << blkptr.getPsize() << " bytes"
           << endl;
        os << hex << blkptr.blk_checksum[0] << ":" << blkptr.blk_checksum[1] << ":" << blkptr.blk_checksum[2] << ":"
           << blkptr.blk_checksum[3] << endl;
    }

    return os;
}
