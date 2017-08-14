/*
 * ZAP.cpp
 * The Sleuth Kit
 *
 * This software is distributed under the Common Public License 1.0
 *
 */

/**
 * \file ZAP.cpp
 * Describes a ZFS ZAP object
 */

#include "ZAP.h"

const uint64_t MICRO_ZAP_MAGIC = 0x8000000000000003;
const uint64_t FAT_ZAP_MAGIC = 0x8000000000000001;
const uint64_t FAT_LEAF_MAGIC = 0x8000000000000000;

using namespace std;

ZAP::ZAP(TSK_ENDIAN_ENUM endian, uint8_t *data, size_t length, bool isDir)
    : isDirectory(isDir), endian(endian), entries() {
    uint64_t magic = read64Bit(endian, data);

    uint64_t mze_value = 0;
    string mze_name = "";
    uint8_t mze_type = 0;
    if (magic == MICRO_ZAP_MAGIC) {
        for (int i = 0; i < (length / 64) - 1; ++i) {
            mze_value = read64Bit(endian, data + 64 + i * 64);
            mze_name = string((char *) data + 64 + 14 + i * 64);
            if ((mze_value != 0) and (mze_name[0] != '\0')) {
                this->entries[mze_name] = mze_value;
            }
        }
    } else if (magic == FAT_ZAP_MAGIC){
        uint64_t zap_num_leafs = read64Bit(endian, data + 64);
        int START_LEAFS = 0x4000;
        for (int i = 0; i < zap_num_leafs; ++i) {
            this->parseFatLeaf(data + START_LEAFS + 0x4000*i);
        }
    } else {
        cout << "Invalid ZAP magic!" << endl;
    }
}

void ZAP::parseFatLeaf(uint8_t* data){
    std::vector<uint16_t> chunk_entries;
    int START_CHUNKS = 48+2*512;

    uint64_t lhr_block_type = read64Bit(this->endian, data);
    if (lhr_block_type != FAT_LEAF_MAGIC) {
        cerr << "Error: Not a valid FAT Leaf as expected!" << endl;
        return;
    }

    for (int i = 0; i < 512; ++i) {
        uint16_t l_hash = read16Bit(this->endian, data + 48 + 2 * i);
        if (l_hash != 0xffff)
            chunk_entries.push_back(l_hash);
    }

    for (std::vector<uint16_t>::iterator it = chunk_entries.begin(); it!= chunk_entries.end(); it++) {
        parseFatLeafChunkEntry(data + START_CHUNKS,(*it));
    }
}

void ZAP::parseFatLeafChunkEntry(uint8_t* data, int chunkEntry){
    uint8_t chunk_type = (data + chunkEntry * 24)[0];

    if (unsigned(chunk_type) != 252)
        cerr << "Error: Not a chunk entry!" << endl;
    uint16_t chunk_next = read16Bit(endian, data + chunkEntry * 24 + 2);
    uint16_t chunk_nameChunk = read16Bit(endian, data + chunkEntry * 24 + 4);
    uint16_t chunk_nameSize = read16Bit(endian, data + chunkEntry * 24 + 6);
    uint16_t chunk_valueChunk = read16Bit(endian, data + chunkEntry * 24 + 8);
    uint16_t chunk_valueSize = read16Bit(endian, data + chunkEntry * 24 + 10);

    std::vector<char> chunkData;

    this->getChunkData(data, chunk_nameChunk * 24, &chunkData);
    string chunk_name = string(chunkData.begin(), chunkData.begin() + chunk_nameSize - 1);
    chunkData.clear();
    this->getChunkData(data, chunk_valueChunk * 24, &chunkData);
    uint64_t  chunk_value = read64Bit(TSK_BIG_ENDIAN,  (uint8_t*) &chunkData[0]);
    this->entries[chunk_name] = chunk_value;

}

void ZAP::getChunkData(uint8_t* data, int offset, std::vector<char>* chunkData){
    uint8_t chunk_type = (data + offset)[0];
    if (chunk_type != 251) {
        cerr << "Error: Not a chunk array!" << endl;
    }

    chunkData->insert(chunkData->end(), (data + offset) + 1, (data + offset) + 22);
    uint16_t chunk_next = read16Bit(this->endian, data + offset + 22);

    if (chunk_next != 0xffff){
        getChunkData(data, chunk_next * 24, chunkData);
    }
}

uint64_t ZAP::getValue(string name) {
    if (this->entries.find(name) == this->entries.end()){
        return FALSE;
    }
    else {
        return this->entries[string(name)];
    }
}

ostream& operator<<(ostream& os, const ZAP& microzap) {
    std::map<string, uint64_t >::const_iterator it;

    for ( it = microzap.entries.begin(); it != microzap.entries.end(); it++ ) {
        os << it->first << " : " << it->second << std::endl ;
    }

    return os;
}

