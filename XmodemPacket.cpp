//
// Created by paolo on 11/1/17.
//

#include <cstring>
#include <boost/asio/detail/array.hpp>
#include <algorithm>
#include "XmodemPacket.h"
#include "Device.h"
#include "Exceptions.h"

using namespace std;

mutex XmodemPacket::crc_mtx;
boost::crc_optimal<16, 0x1021, 0, 0, false, false> XmodemPacket::crc;

XmodemPacket::XmodemPacket(uint8_t pktnum) {
    content.block_num = pktnum;
    content.block_num_neg = ~pktnum;
    content.start = XMODEM_SOH;
    memset(content.payload, 0, XMODEM_DATA_SIZE);
}

XmodemPacket XmodemPacket::next() {
    return XmodemPacket(static_cast<uint8_t>(content.block_num + 1));
}

void XmodemPacket::read_from_binfile(std::ifstream &file) {
    file.read(reinterpret_cast<char *>(content.payload), XMODEM_DATA_SIZE);
    auto bytes_read = file.gcount();
    if (bytes_read < XMODEM_DATA_SIZE) { //packet needs padding
        if (file.eof()) {
            memset(content.payload + bytes_read, 0xff, static_cast<size_t>(XMODEM_DATA_SIZE - bytes_read));
        } else {
            throw FileIOException("File reading interrupted by astral phenomena");
        }
    }
}

void XmodemPacket::compute_crc() {
    crc_mtx.lock();
    crc.reset();
    crc.process_bytes(content.payload, XMODEM_DATA_SIZE);
    content.crc = static_cast<uint16_t>(crc.checksum());
    crc_mtx.unlock();
    content.crc = static_cast<uint16_t>((content.crc >> 8) & 0xFF | (content.crc << 8) & 0xFF00);
}

char* XmodemPacket::get_content() {
    return reinterpret_cast<char *>(&content);
}
