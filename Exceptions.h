//
// Created by paolo on 11/2/17.
//

#ifndef WANDSTEM_FLASH_UTILITY_EXCEPTIONS_H
#define WANDSTEM_FLASH_UTILITY_EXCEPTIONS_H

#include <ios>
#include <string>
#include "Device.h"

class DeviceNotFoundException : public std::ios_base::failure {
public:
    explicit DeviceNotFoundException(const std::string &arg) : failure(arg) {}
};

class BinaryNotFoundException : public std::ios_base::failure {
public:
    explicit BinaryNotFoundException(const std::string &arg) : failure(arg) {}
};

class XmodemTransmissionException : public std::ios_base::failure {
public:
    explicit XmodemTransmissionException(const std::string &arg) : failure(arg) {}
};

class FileIOException : public std::ios_base::failure {
public:
    explicit FileIOException(const std::string &arg) : failure(arg) {}
};

class WontExecuteException : public std::ios_base::failure {
public:
    explicit WontExecuteException(const std::string &arg) : failure(arg) {}
};

#endif //WANDSTEM_FLASH_UTILITY_EXCEPTIONS_H
