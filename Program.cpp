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

#include "Program.h"
#include "Exceptions.h"
#include "Device.h"
#include <boost/program_options.hpp>
#include <boost/algorithm/string.hpp>
#include <csignal>
#include <sys/stat.h>

namespace po = boost::program_options;
using namespace std;

istream &operator>>(istream &in, Program::flash_mode &unit) {
    string token;
    in >> token;
    std::transform(token.begin(), token.end(), token.begin(), ::tolower);
    if (token == "u" || token == "usb")
        unit = Program::flash_mode::USB;
    else if (token == "s" || token == "serial")
        unit = Program::flash_mode::SERIAL;
    else if (token == "a" || token == "auto")
        unit = Program::flash_mode::AUTO;
    else
        in.setstate(ios_base::failbit);
    return in;
}

Program &Program::get_instance() {
    static Program instance;
    return instance;
}

void Program::init(int argc, const char *argv[]) {

    // Declare the supported options.
    po::options_description total("Arguments");

    po::options_description required_options("Required (at least one must be specified)");
    required_options.add_options()
            ("help,h", "Produces this message")
            ("print,p", "Enables the output printing mode")
            ("flash,f", po::value<string>(), "Flashes the specified binary file");

    po::options_description connection_options("Connection");
    connection_options.add_options()
            ("mode,m", po::value<flash_mode>(),
             "Indicates how the board is connected:\n - a for auto (default);\n - u for USB;\n - s for serial adapter")
            ("device,d", po::value<string>(), "Specifies the tty device path\nDefault:\n    USB mode: \t/dev/ttyACM0\n    serial mode: \t/dev/ttyUSB0")
            ("baud,b", po::value<int>(), "Specifies the baud rate to be used\nDefault:\n    USB mode: \t115200\n    serial mode: \t9600");
    total.add(required_options).add(connection_options);

    po::variables_map vm;
    po::store(po::parse_command_line(argc, argv, total), vm);
    po::notify(vm);

    if (vm.count("help") || !(vm.count("flash") + vm.count("print"))) {
        cout << total << "\n";
        throw WontExecuteException("Asked for help");
    }

    //decode params

    args.print = static_cast<bool>(vm.count("print"));

    if (vm.count("flash"))
        args.bin_path = vm["flash"].as<string>();

    if (vm.count("mode"))
        args.flash_mode = vm["mode"].as<flash_mode>();

    //if device is selected, mode is ignored

    if (vm.count("device"))
        args.device_path = vm["device"].as<string>();

    //if baud is specified, default is ignored

    if (vm.count("baud"))
        args.baud = vm["baud"].as<unsigned int>();

    signal(SIGINT, stop);

    //init the device
    if (args.device_path.empty()) {
        //check for the mode
        switch (args.flash_mode) {
            case USB:
                device = new USBDevice();
                break;
            case SERIAL:
                device = new UARTDevice();
                break;
            case AUTO:
            default:
                struct stat buffer;
                if (stat("/dev/ttyACM0", &buffer) == 0)
                    device = new USBDevice();
                else if (stat("/dev/ttyUSB0", &buffer) == 0)
                    device = new UARTDevice();
                else
                    throw DeviceNotFoundException(
                            "Device not found using auto discovery. Please specify the device path.");
                break;
        }
    } else {
        if (boost::to_upper_copy(args.device_path).find("ACM")) {
            if (args.baud == -1)
                device = new USBDevice(args.device_path);
            else
                device = new USBDevice(args.device_path, args.baud);
        } else {
            if (args.baud == -1)
                device = new UARTDevice(args.device_path);
            else
                device = new UARTDevice(args.device_path, args.baud);
        }
    }
}

void Program::flash_if_needed() {
    if (args.bin_path.empty()) return;
    try {
        device->flash(args.bin_path);
    } catch (XmodemTransmissionException &ex) {
        cout << "Xmodem transmission error:" << endl << ex.what() << ". Flash operation aborted." << endl;
    } catch (DeviceNotFoundException &ex) {
        cout << "Error while establishing communication with device:" << endl << ex.what()
             << ". Flash operation aborted." << endl;
    } catch (BinaryNotFoundException &ex) {
        cout << "Error opening the binary image file:" << endl << ex.what() << ". Flash operation aborted." << endl;
    } catch (FileIOException &ex) {
        cout << "Binary file reading error:" << endl << ex.what() << ". Flash operation aborted." << endl;
    } catch (ios::failure &ex) {
        cout << "Physical communication with the device error:" << endl << ex.what() << ". Flash operation aborted."
             << endl;
    }
}

void Program::read_to_end() {
    if (!args.print) return;
    if (auto *p = dynamic_cast<USBDevice*>(device)) {
        cout << "Cannot read standard output from a device connected in USB mode." << endl;
        return;
    }
    if(!device->open_comm())
        cout << "Generic error while enstablishing communication with the device" << endl;
    for (; running;);
        device->read_and_print<char>();
}

void Program::stop(int sig) {
    auto& p = Program::get_instance();
    p.device->close_comm();
    p.running = false;
}
