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
 *   GNU General Public License for more details.                          *                                                      *
 *                                                                         *
 *   You should have received a copy of the GNU General Public License     *
 *   along with this program; if not, see <http://www.gnu.org/licenses/>   *
 ***************************************************************************/

#include "Device.h"
#include "XmodemPacket.h"
#include "Exceptions.h"
#include <sys/stat.h>

using namespace std;


bool Device::open_comm() {
    if (comm_opened) return true;
    serial_stream.exceptions(ios::badbit | ios::failbit);
    comm_opened = serial_stream.good();
    return comm_opened;
}

bool UARTDevice::open_comm() {
    if (comm_opened) return true;
    serial_stream.exceptions(ios::badbit | ios::failbit);
    comm_opened = serial_stream.good();
    if (comm_opened){
        //send a 'U' for autobaud the interface
        serial_stream << "U" << flush;
        if (!detect_bootloader_mode(false))
            throw DeviceNotFoundException("Device not connected or not in bootloader mode");
        return true;
    } else return false;
}

template<>
std::string Device::read_and_print<std::string>() {
    std::string retval;
    getline(serial_stream, retval);
    std::cout << retval;
    return retval;
}

bool Device::check_device_present() {
    struct stat buffer{};
    return static_cast<bool>(stat(path.c_str(), &buffer) == 0);
}

bool Device::detect_bootloader_mode(bool strict) {
    if(!check_output(strict? BOOTLOADER_REGEX_STRICT: BOOTLOADER_REGEX_NOSTRICT, chrono::milliseconds(1000))){
        serial_stream << "i" << flush;
        return check_output(BOOTLOADER_REGEX_STRICT, chrono::milliseconds(1000));
    }
    return true;
}

bool Device::prepare_flash() {
    if (!check_device_present())
        throw DeviceNotFoundException("Device not found");
    bool retval = open_comm();

    cout << " :: Enabling firmware upload mode ::" << endl;
    //start the upload mode of the bootloader
    serial_stream << "u" << flush;
    return retval && check_output("^Ready(\\r)?$", chrono::milliseconds(1000));
}

void Device::send_byte(uint8_t data, bool flush) {
    char pkt[] = {static_cast<char>(data)};
    serial_stream.write(pkt, sizeof(pkt));
    if(flush) serial_stream.flush();
}

void Device::flash(std::string filename) {
    //check the binary image file exists
    struct stat stat_buffer{};
    if (stat(filename.c_str(), &stat_buffer))
        throw BinaryNotFoundException("Binary not found in the specified path");
    cout << " :: Loading binary image file...";
    ifstream file(filename, std::ios::binary);
    if (!file)
        throw BinaryNotFoundException("Binary not found in the specified path");
    cout << "loaded! ::" << endl;

    if (!prepare_flash())
        throw DeviceNotFoundException("Broken pipe");

    //flash procedure by http://web.mit.edu/6.115/www/amulet/xmodem.htm

    uint8_t reply;
    bool ack = false;
    XmodemPacket pkt;
    //wait for 'C' meaning the device is accepting an XMODEM transfer
    for (int retry = 0; !ack && retry < MAX_RETRANSMISSION; retry++) {
        reply = read_and_print<uint8_t>();
        ack = reply == XMODEM_NCG;
    }
    if (!ack)
        throw XmodemTransmissionException("The device is not accepting the transmission using XMODEM protocol");
    cout << endl << " :: Ready to receive data in CRC mode. Starting to flash the image ::" << endl;
    int column = 0;
    int num_pkts;
    for (num_pkts = 0; file; num_pkts++) {
        //compose a packet
        try {
            pkt.read_from_binfile(file);
        } catch (FileIOException &ex) {
            send_byte(XMODEM_CAN);
            send_byte(XMODEM_CAN);
            send_byte(XMODEM_CAN);
            throw ex;
        }
        pkt.compute_crc();
        ack = false;
        //send the packet
        for (int retry = 0; !ack && retry < MAX_RETRANSMISSION; retry++) {
            char* pkt_content = pkt.get_content();
            serial_stream.write(pkt_content, XMODEM_PACKET_SIZE);
            serial_stream.flush();
            serial_stream.read(reinterpret_cast<char*>(&reply), sizeof(reply));
            if (retry) cout << '\b' << flush;
            else if (column++ == 80) {
                column = 1;
                cout << endl;
            }
            switch (reply) {
                case XMODEM_ACK: //packet acknowledged
                    cout << '.' << flush;
                    ack = true;
                    break;
                case XMODEM_CAN: //cancelled by target
                    cout << 'C' << flush;
                    serial_stream.read(reinterpret_cast<char *>(&reply), 1);
                    if (reply == XMODEM_CAN) {
                        serial_stream.read(reinterpret_cast<char *>(&reply), 1);
                        send_byte(XMODEM_ACK);
                        cout << endl;
                        throw XmodemTransmissionException("Transmission cancelled by target");
                    }
                    break;
                case XMODEM_NAK: //otherwise retry
                    cout << 'N' << flush;
                default:
                    break;
            }
        }
        //too many errors, aborting
        if (!ack) {
            send_byte(XMODEM_CAN);
            send_byte(XMODEM_CAN);
            send_byte(XMODEM_CAN);
            cout << endl;
            throw XmodemTransmissionException("Too many errors while sending packet, transmission aborted");
        }
        pkt = pkt.next();
    }
    ack = false;
    cout << endl << " :: End of transmission, " << num_pkts << " packets sent ::" << endl;
    //communicate the end of the transmission and wait for its ack
    for (int retry = 0; !ack && retry < 2 * MAX_RETRANSMISSION; retry++) {
        send_byte(XMODEM_EOT);
        serial_stream.read(reinterpret_cast<char *>(&reply), 1);
        ack = reply == XMODEM_ACK;
    }
    if (ack) {
        cout << " :: Rebooting the device... ::" << endl;
        serial_stream << "b" << flush;
        return;
    }
    throw XmodemTransmissionException("Remote target did not ACK end of transmission");
}

void Device::close_comm() {
    if (!comm_opened) return;
    comm_opened = false;
    serial_stream.close();
}
