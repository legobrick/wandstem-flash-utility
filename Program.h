//
// Created by paolo on 10/29/17.
//

#ifndef WANDSTEM_FLASH_UTILITY_PROGRAM_H
#define WANDSTEM_FLASH_UTILITY_PROGRAM_H

#include <string>
#include <ios>
#include "Device.h"


/**
 * This class models the Program during its phases.
 * It is a singleton class, whose instance is obtainable by the Program::get_instance method.
 */
class Program {
public:
    ///The possible values of the flash mode program option.
    enum flash_mode {
        USB, SERIAL, AUTO
    };

private:
    /**
     * Default constructor.
     */
    Program() = default;

    ///The possible arguments with which the program was invoked.
    struct arguments_t {
        bool print = false;
        std::string bin_path = "";
        Program::flash_mode flash_mode = AUTO;
        std::string device_path;
        unsigned int baud = static_cast<unsigned int>(-1);
    } args;

    ///The instance of the Device to which we will interface.
    Device *device = nullptr;

    ///The controller variable for program interruption
    bool running = true;

public:
    Program(Program const &) = delete;

    void operator=(Program const &) = delete;

    /**
     * Returns the singleton instance of the class.
     * \return Program instance
     */
    static Program &get_instance();

    /**
     * Initializes the Program by parsing the arguments, determining and instantiating the correct device with the specified parameters.
     * \throws DeviceNotFoundException if auto mode is selected and no device is found.
     * \throws WontExecuteException if any mode was selected.
     * \param argc the number of arguments
     * \param argv the argument strings
     * \return
     */
    void init(int argc, const char *argv[]);

    /**
     * Initializes or re-initializes the device with the specified parameters, with the optionality of the timeout
     * \param infinite_timeout if the device should have an infinite timeout or not.
     * \return
     */
    void init_device(bool infinite_timeout = false);

    /**
     * Flashes the device is specified in the arguments.
     * \return
     */
    void flash_if_needed();

    /**
     * Reads the device stream until the process is not stopped, if the print argument was specified.
     * \return
     */
    void read_to_end();

    /**
     * Stops the process.
     * \return
     */
    static void stop(int sig);
};


#endif //WANDSTEM_FLASH_UTILITY_PROGRAM_H
