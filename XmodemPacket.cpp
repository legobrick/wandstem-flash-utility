/***************************************************************************
 *   Copyright (C) 2017 by Paolo Polidori                                  *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 *   This program is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU General Public License for more details.                          *
 *                                                                         *
 *   You should have received a copy of the GNU General Public License     *
 *   along with this program; if not, see <http://www.gnu.org/licenses/>   *
 ***************************************************************************/

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
    content.start = xmodemSoh;
    memset(content.payload, 0, xmodemDataSize);
}

XmodemPacket XmodemPacket::next() {
    return XmodemPacket(static_cast<uint8_t>(content.block_num + 1));
}

void XmodemPacket::read_from_binfile(std::ifstream &file) {
    file.read(reinterpret_cast<char *>(content.payload), xmodemDataSize);
    auto bytes_read = file.gcount();
    if (bytes_read < xmodemDataSize) { //packet needs padding
        if (file.eof()) {
            memset(content.payload + bytes_read, 0xff, static_cast<size_t>(xmodemDataSize - bytes_read));
        } else {
            throw FileIOException("File reading interrupted by astral phenomena");
        }
    }
}

void XmodemPacket::compute_crc() {
    crc_mtx.lock();
    crc.reset();
    crc.process_bytes(content.payload, xmodemDataSize);
    content.crc = static_cast<uint16_t>(crc.checksum());
    crc_mtx.unlock();
    content.crc = static_cast<uint16_t>((content.crc >> 8) & 0xFF | (content.crc << 8) & 0xFF00);
}

char* XmodemPacket::get_content() {
    return reinterpret_cast<char *>(&content);
}
