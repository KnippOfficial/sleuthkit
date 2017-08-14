/*
 * NVList.cpp
 * The Sleuth Kit
 *
 * This software is distributed under the Common Public License 1.0
 *
 */

/**
 * \file NVList.cpp
 * Describes the NVList object in ZFS
 */

#include "NVList.h"

using namespace std;

NVList::NVList(TSK_ENDIAN_ENUM endian, uint8_t *data, int nestedLevel)
        : endian(endian), nestedLevel(nestedLevel) {
    uint32_t nvLength;
    int index = 0;
    beginningNV = read64Bit(endian, data);
    index += 8;

    if (beginningNV != 1)
        cout << "Broken NV List!" << endl;

    while (true) {
        nvLength = read32Bit(endian, data + index);
        if (nvLength != 0) {
            parseNVElement(data, index, nvLength);
            index += nvLength;
        } else if (read64Bit(endian, data + index) == 0) {
            break;
        }
    }

    //return index + 8 for last 8 byte of zeros
    size = index + 8;
}

NVList::~NVList() {
    for (auto &lists : elements_nvListArray) {
        for (auto &it : lists.second) {
            delete it;
        }
    }
}

void NVList::parseNVElement(uint8_t *data, int index, uint64_t length) {
    uint64_t nameLength = read32Bit(endian, data + index + 8);
    nameLength = roundup(nameLength, 4);
    string name;
    name = string((char *) data + index + 12);
    uint32_t dataType = read32Bit(endian, data + index + nameLength + 12);
    uint32_t noElements = read32Bit(endian, data + index + nameLength + 16);

    if (dataType == 1) {
        uint32_t value = read32Bit(endian, data + index + nameLength + 16);
        elements_int[name] = value;
    } else if (dataType == 8) {
        uint64_t value = read64Bit(endian, data + index + nameLength + 20);
        elements_int[name] = value;
    } else if (dataType == 9) {
        uint32_t valueLength = read32Bit(endian, data + index + nameLength + 20);
        valueLength = roundup(valueLength, 4);
        string value = string((char *) data + index + nameLength + 24);
        elements_string[name] = value;
    } else if (dataType == 19 or dataType == 20) {
        int offset = 0;
        vector<NVList *> lists;
        NVList *list;
        for (int i = 0; i < noElements; i++) {
            list = new NVList(endian, data + index + nameLength + 20 + offset, nestedLevel + 1);
            lists.push_back(list);
            offset += list->getSize();
        }
        elements_nvListArray.insert(std::pair<string, vector<NVList *>>(name, lists));
    }
}

string NVList::getStringValue(string name) {
    if (elements_string.find(name) != elements_string.end())
        return elements_string[string(name)];
    else
        return FALSE;
}

uint64_t NVList::getIntValue(string name) {
    if (elements_int.find(name) != elements_int.end())
        return elements_int[string(name)];
    else
        return FALSE;
}

vector<NVList *> NVList::getNVListArray(string name) {
    if (elements_nvListArray.find(name) != elements_nvListArray.end())
        return elements_nvListArray[string(name)];
    else
        return vector<NVList *>();
}

ostream &operator<<(ostream &os, const NVList &nvlist) {
    int tabLevel = nvlist.nestedLevel;

    for (auto it_int = nvlist.elements_int.begin(); it_int != nvlist.elements_int.end(); it_int++) {
        os << std::string(tabLevel * 4, ' ') << it_int->first
           << " --> "
           << it_int->second
           << std::endl;
    }

    for (auto it_string = nvlist.elements_string.begin(); it_string != nvlist.elements_string.end(); it_string++) {
        os << std::string(tabLevel * 4, ' ') << it_string->first
           << " --> "
           << it_string->second
           << std::endl;
    }

    for (auto it_list = nvlist.elements_nvlist.begin(); it_list != nvlist.elements_nvlist.end(); it_list++) {
        os << it_list->first
           << " --> " << endl
           << *it_list->second
           << std::endl;
    }

    return os;
}