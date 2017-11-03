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
#include "serial-port/6_stream/serialstream.h"

#define MAX_RETRANSMISSION 5

#define BOOTLOADER_REGEX_STRICT "^BOOTLOADER version (.+) Chip ID ([0-9A-F]+)(\\r)?$"
#define BOOTLOADER_REGEX_NOSTRICT "^(BOOTLOADER version (.+) Chip ID ([0-9A-F]+)|\\?)(\\r)?$"


/**
 * This class models the Device with which the program will operate.
 */
class Device {

private:
    /**
     * Checks that the serial outputs a string matching with the provided one within a certain timeout.
     * @param regex_string the string to be match against.
     * @param timeout the maximum waiting period.
     * @return if the output matched.
     */
    bool check_output(std::string regex_string, std::chrono::milliseconds timeout);

protected:

    /// The path to the device.
    std::string path;

    /// The baud used in the serial connection.
    unsigned int baud;

    /// The stream related to the device.
    SerialStream *serial_stream;

    /// If the communication with the device is opened.
    bool comm_opened = false;

    /// The pointer to the original output buffer of the device.
    std::streambuf *outbuf = nullptr;

    /**
     * Constructor. Initializes the object.
     * \param path the path to the device
     * \param baud the baud to be used for the serial communication
     * \return
     */
    Device(std::string path, unsigned int baud) : path(std::move(path)), baud(baud) {};

    /**
     * Checks that the device is present at the specified Device::path.
     * \return if the device is present.
     */
    bool check_device_present();

    /**
     * Checks wether the device is connected in bootloader mode.
     * \return if the device is in bootloader mode.
     */
    bool detect_bootloader_mode(bool strict = true);

    /**
     * Prepares the device to be flashed.
     * It initializes the bootloader for accepting binary images.
     * \return if the device is ready to be flashed.
     */
    virtual bool prepare_flash();

public:

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
    T read_and_print();

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
     * Redirects the stream output to another stream.
     * Useful for printing device debug strings to screen or file.
     * \throws DeviceNotFoundException If the device did not respond.
     * \param targetbuf The buffer to which redirect the output.
     * \return
     */
    void redirect_output(std::ostream &targetbuf);

    /**
     * Restores the device output stream.
     * \return
     */
    void reset_output();

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

public:
    /**
     * Constructor. Initializes a default serial connected Wandstem: /dev/USB0 with 115200 baud rate.
     * \return
     */
    UARTDevice() : Device("/dev/ttyUSB0", 115200) {};

    /**
     * Opens the SerialStream with the device and does the autobaud.
     * \return if the stream was opened successfully.
     */
    bool open_comm() override;

    /**
     * Constructor. Initializes a default serial connected Wandstem with the specified path and 115200 baud rate.
     * \param path The path to the device.
     * \return
     */
    explicit UARTDevice(std::string path) : Device(std::move(path), 115200) {};

    /**
     * Constructor. Initializes a default serial connected Wandstem with the specified path and the specified baud rate.
     * \param path The path to the device.
     * \param baud The baud rate for communicating with the device.
     * \return
     */
    UARTDevice(std::string path, unsigned int baud) : Device(std::move(path), baud) {};
};

/**
 * This class models a USB connected Device with which the program will operate.
 */
class USBDevice : public Device {

public:
    /**
     * Constructor. Initializes a default USB connected Wandstem: /dev/ttyACM0 with 9600 baud rate.
     * \return
     */
    USBDevice() : Device("/dev/ttyACM0", 9600) {};

    /**
     * Constructor. Initializes a default USB connected Wandstem with the specified path and 9600 baud rate.
     * \param path The path to the device.
     * \return
     */
    explicit USBDevice(std::string path) : Device(std::move(path), 9600) {};

    /**
     * Constructor. Initializes a default USB connected Wandstem with the specified path and the specified baud rate.
     * \param path The path to the device.
     * \param baud The baud rate for communicating with the device.
     * \return
     */
    USBDevice(std::string path, unsigned int baud) : Device(std::move(path), baud) {};
};


#endif //WANDSTEM_FLASH_UTILITY_DEVICE_H
