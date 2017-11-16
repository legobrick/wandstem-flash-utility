//
// Created by paolo on 10/30/17.
//

#ifndef WANDSTEM_FLASH_UTILITY_DEVICE_H
#define WANDSTEM_FLASH_UTILITY_DEVICE_H

#include <string>
#include <utility>
#include <ios>
#include <boost/date_time.hpp>
#include <chrono>
#include <thread>
#include <condition_variable>
#include <regex>
#include "serial-port/6_stream/serialstream.h"

#define MAX_RETRANSMISSION 5
#define DEVICE_TIMEOUT_MSEC 2500

#define BOOTLOADER_REGEX_STRICT "^BOOTLOADER version (.+) Chip ID ([0-9A-F]+)(\\r)?$"
#define BOOTLOADER_REGEX_NOSTRICT "^(BOOTLOADER version (.+) Chip ID ([0-9A-F]+)|\\?)(\\r)?$"


/**
 * This class models the Device with which the program will operate.
 */
class Device {

protected:
    /**
     * Checks that the serial outputs a string matching with the provided one within a certain timeout.
     * @param regex_string the string to be match against.
     * @param timeout the maximum waiting period.
     * @return if the output matched.
     */
    template<typename _Rep, typename _Period>
    bool check_output(const std::string &regex_string, const std::chrono::duration<_Rep, _Period> &timeout) {
        std::chrono::time_point<std::chrono::system_clock> end = std::chrono::system_clock::now() + timeout;
        std::regex r(regex_string);
        bool detected = false;
        while (!detected && std::chrono::system_clock::now() < end) {
            std::string s = read_and_print<std::string>();
            if (regex_match(s, r))
                detected = true;
        }

        return detected;
    }

    /// The path to the device.
    std::string path;

    /// The baud used in the serial connection.
    unsigned int baud;

    /// The stream related to the device.
    SerialStream serial_stream;

    /// If the communication with the device is opened.
    bool comm_opened = false;

    /**
     * Constructor. Initializes the object.
     * \param path the path to the device
     * \param baud the baud to be used for the serial communication
     * \param infinite_timeout if the timeout should be limited to 2,5s or infinite
     * \return
     */
    Device(std::string path, unsigned int baud, bool infinite_timeout = false) : path(std::move(path)), baud(baud), serial_stream(
            SerialOptions(this->path, baud, boost::posix_time::milliseconds(infinite_timeout? 0 : DEVICE_TIMEOUT_MSEC))) {};

    /**
     * Checks that the device is present at the specified Device::path.
     * \return if the device is present.
     */
    bool check_device_present();

    /**
     * Checks wether the device is connected in bootloader mode.
     * \param strict if the check doesn't also admit the '?' for unrecognized input
     * \return if the device is in bootloader mode.
     */
    bool detect_bootloader_mode(bool strict = true);

    /**
     * Prepares the device to be flashed.
     * It initializes the bootloader for accepting binary images.
     * \return if the device is ready to be flashed.
     */
    virtual bool prepare_flash();

    /**
     * Sends a raw byte to the device.
     * \param data the raw byte to be sent
     * \param path if the stream needs to be flushed
     * \return
     */
    void send_byte(uint8_t data, bool flush = true);

public:

    virtual ~Device() = default;

    /**
     * Opens the SerialStream with the device.
     * \throws DeviceNotFoundException If the device unexpectedly stop responding.
     * \return if the stream was opened successfully.
     */
    virtual bool open_comm();

    /**
     * Reads something from the device and prints it to screen.
     * \tparam T the type of parameter to be read.
     * \return the read value.
     */
    template<class T>
    T read_and_print() {
        T retval;
        serial_stream.read(reinterpret_cast<char*>(&retval), sizeof(retval));
        std::cout << retval << std::flush;
        return retval;
    }

    /**
     * \internal
     * Write to serial port.
     * \throws XmodemTransmissionException If errors at XMODEM protocol level occurred.
     * \throws DeviceNotFoundException If the device unexpectedly stop responding.
     * \throws BinaryNotFoundException If the binary file was not found.
     * \throws FileIOException If there were problems opening or reading the binary file.
     * \throws ios::failure If the stream transmission to the device returned an error.
     * \param filename The binary image file path.
     * \return
     */
    void flash(std::string filename);

    /**
     * Closes the stream communication with the device, if opened.
     * \return
     */
    void close_comm();
};

/**
 * This class models a serial connected Device with which the program will operate.
 */
class UARTDevice : public Device {

private:
    /**
     * Prepares the device to be flashed.
     * It initializes the bootloader for accepting binary images.
     * \return if the device is ready to be flashed.
     */
    bool prepare_flash() override;

public:
    /**
     * Constructor. Initializes a default serial connected Wandstem: /dev/USB0 with 115200 baud rate.
     * \param infinite_timeout if the timeout should be limited to 2,5s or infinite
     * \return
     */
    explicit UARTDevice(bool infinite_timeout = false) : Device("/dev/ttyUSB0", 115200, infinite_timeout) {};

    /**
     * Constructor. Initializes a default serial connected Wandstem with the specified path and 115200 baud rate.
     * \param path The path to the device.
     * \param infinite_timeout if the timeout should be limited to 2,5s or infinite
     * \return
     */
    UARTDevice(std::string path, bool infinite_timeout = false) : Device(std::move(path), 115200, infinite_timeout) {};

    /**
     * Constructor. Initializes a default serial connected Wandstem with the specified path and the specified baud rate.
     * \param path The path to the device.
     * \param baud The baud rate for communicating with the device.
     * \param infinite_timeout if the timeout should be limited to 2,5s or infinite
     * \return
     */
    UARTDevice(std::string path, unsigned int baud, bool infinite_timeout = false) : Device(std::move(path), baud, infinite_timeout) {};
};

/**
 * This class models a USB connected Device with which the program will operate.
 */
class USBDevice : public Device {

public:
    /**
     * Constructor. Initializes a default USB connected Wandstem: /dev/ttyACM0 with 9600 baud rate.
     * \param infinite_timeout if the timeout should be limited to 2,5s or infinite
     * \return
     */
    explicit USBDevice(bool infinite_timeout = false) : Device("/dev/ttyACM0", 9600, infinite_timeout) {};

    /**
     * Constructor. Initializes a default USB connected Wandstem with the specified path and 9600 baud rate.
     * \param path The path to the device.
     * \param infinite_timeout if the timeout should be limited to 2,5s or infinite
     * \return
     */
    USBDevice(std::string path, bool infinite_timeout = false) : Device(std::move(path), 9600, infinite_timeout) {};

    /**
     * Constructor. Initializes a default USB connected Wandstem with the specified path and the specified baud rate.
     * \param path The path to the device.
     * \param baud The baud rate for communicating with the device.
     * \param infinite_timeout if the timeout should be limited to 2,5s or infinite
     * \return
     */
    USBDevice(std::string path, unsigned int baud, bool infinite_timeout = false) : Device(std::move(path), baud, infinite_timeout) {};
};


#endif //WANDSTEM_FLASH_UTILITY_DEVICE_H
