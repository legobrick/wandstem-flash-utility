//
// Created by paolo on 10/30/17.
//

#include "Device.h"
#include "XmodemPacket.h"
#include "Exceptions.h"
#include <sys/stat.h>
#include <regex>
#include <thread>
#include <condition_variable>

using namespace std;


bool Device::open_comm() {
    if (comm_opened) return true;
    SerialOptions options(path, baud, boost::posix_time::seconds(2));
    serial_stream = new SerialStream(options);
    serial_stream->exceptions(ios::badbit | ios::failbit);
    outbuf = serial_stream->rdbuf();
    comm_opened = serial_stream->good();
    return comm_opened;
}

bool UARTDevice::open_comm() {
    if (comm_opened) return true;
    SerialOptions options(path, baud, boost::posix_time::seconds(0)); //TODO set timeout
    serial_stream = new SerialStream(options);
    serial_stream->exceptions(ios::badbit | ios::failbit);
    outbuf = serial_stream->rdbuf();
    comm_opened = serial_stream->good();
    if (comm_opened){
        //send a 'U' for autobaud the interface
        *serial_stream << "U" << flush;
        if (!detect_bootloader_mode(false))
            throw DeviceNotFoundException("Device not connected or not in bootloader mode");
        return true;
    } else return false;
}

template<class T>
T Device::read_and_print() {
    T retval;
    serial_stream->read(reinterpret_cast<char*>(&retval), sizeof(retval));
    cout << retval;
    return retval;
}

template<>
string Device::read_and_print<string>() {
    string retval;
    getline(*serial_stream, retval);
    cout << retval;
    return retval;
}

bool Device::check_device_present() {
    struct stat buffer{};
    return static_cast<bool>(stat(path.c_str(), &buffer) == 0);
}

bool Device::detect_bootloader_mode(bool strict) {
    if(!check_output(strict? BOOTLOADER_REGEX_STRICT: BOOTLOADER_REGEX_NOSTRICT, chrono::milliseconds(1000))){
        *serial_stream << "i" << flush;
        return check_output(BOOTLOADER_REGEX_STRICT, chrono::milliseconds(1000));
    }
    return true;
}

bool Device::prepare_flash() {
    if (!check_device_present())
        throw DeviceNotFoundException("Device not found");
    bool retval = open_comm();
    if (!detect_bootloader_mode(false))
        throw DeviceNotFoundException("Device not connected or not in bootloader mode");

    //start the upload mode of the bootloader
    *serial_stream << "u" << flush;
    return retval && check_output("^Ready(\\r)?$", chrono::milliseconds(1000));
}

void Device::flash(std::string filename) {
    //check the binary image file exists
    struct stat stat_buffer{};
    if (stat(filename.c_str(), &stat_buffer))
        throw BinaryNotFoundException("Binary not found in the specified path");
    cout << "Loading file... " << endl;
    ifstream file(filename, std::ios::binary);
    if (!file)
        throw BinaryNotFoundException("Binary not found in the specified path");

    if (!prepare_flash())
        throw DeviceNotFoundException("Broken pipe");

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
    while (file) {
        //compose a packet
        try {
            pkt.read_from_binfile(file);
        } catch (FileIOException &ex) {
            *serial_stream << XMODEM_CAN << XMODEM_CAN << XMODEM_CAN << flush;
            throw ex;
        }
        pkt.compute_crc();
        ack = false;
        //send the packet
        for (int retry = 0; !ack && retry < MAX_RETRANSMISSION; retry++) {
            char* pkt_content = pkt.get_content();
            serial_stream->write(pkt_content, XMODEM_PACKET_SIZE);
            serial_stream->flush();
            serial_stream->read(reinterpret_cast<char *>(&reply), 1);
            switch (reply) {
                case XMODEM_ACK: //packet acknowledged
                    cout << "ACK" << endl;
                    ack = true;
                    break;
                case XMODEM_CAN: //cancelled by target
                    serial_stream->read(reinterpret_cast<char *>(&reply), 1);
                    if (reply == XMODEM_CAN) {
                        *serial_stream << XMODEM_ACK << flush;
                        throw XmodemTransmissionException("Transmission cancelled by target");
                    }
                    break;
                case XMODEM_NAK: //otherwise retry
                    cout << "NAK";
                default:
                    break;
            }
        }
        //too many errors, aborting
        if (!ack) {
            *serial_stream << XMODEM_CAN << XMODEM_CAN << XMODEM_CAN << flush;
            throw XmodemTransmissionException("Too many errors while sending packet, transmission aborted");
        }
        pkt = pkt.next();
    }
    ack = false;
    //communicate the end of the transmission and wait for its ack
    for (int retry = 0; !ack && retry < 2 * MAX_RETRANSMISSION; retry++) {
        *serial_stream << XMODEM_EOT << flush;
        serial_stream->read(reinterpret_cast<char *>(&reply), 1);
        ack = reply == XMODEM_ACK;
    }
    if (ack) return;
    throw XmodemTransmissionException("Remote target did not ACK end of transmission");
}

void Device::redirect_output(std::ostream &targetbuf) {
    if (!open_comm())
        throw DeviceNotFoundException("Generic error while enstablishing communication with the device");
    serial_stream->rdbuf(targetbuf.rdbuf());
}

void Device::reset_output() {
    serial_stream->rdbuf(outbuf);
}

void Device::close_comm() {
    if (!comm_opened) return;
    comm_opened = false;
    serial_stream->close();
}

bool Device::check_output(std::string regex_string, std::chrono::milliseconds timeout) {
    mutex m;
    condition_variable cv;

    std::thread t([&m, &cv, &regex_string, this]() {
        regex r(regex_string);
        for (bool detected = false; !detected;) {
            string s = read_and_print<string>();
            if (regex_match(s, r))
                detected = true;
        }
        cv.notify_one();
    });

    t.detach();

    {
        std::unique_lock<std::mutex> l(m);
        return cv.wait_for(l, timeout) <= std::cv_status::timeout;
    }
}
