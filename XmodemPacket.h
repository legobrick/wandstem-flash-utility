//
// Created by paolo on 11/1/17.
//

#ifndef WANDSTEM_FLASH_UTILITY_XMODEMPACKET_H
#define WANDSTEM_FLASH_UTILITY_XMODEMPACKET_H

#include <ios>
#include <fstream>
#include <boost/crc.hpp>
#include <mutex>

enum {
    xmodemSoh=1,
    xmodemEot=4,
    xmodemAck=6,
    xmodemNak=21,
    xmodemCan=24,
    xmodemNcg=67
};

static const int xmodemDataSize=128;
static const int xmodemPacketSize=133;


class XmodemPacket {
private:
    /// The packet structure to be populated, serialized and transmitted.
    struct xmodem_chunk { //total size 133 bytes
        uint8_t start;
        uint8_t block_num;
        uint8_t block_num_neg;
        uint8_t payload[xmodemDataSize];
        uint16_t crc;
    } __attribute__((packed)) content;

    /**
     * Constructor.
     * \param pktnum The progressive packet number.
     * \return
     */
    explicit XmodemPacket(uint8_t pktnum);

    /// The CRC calculation utility.
    static boost::crc_optimal<16, 0x1021, 0, 0, false, false> crc;

    /// A mutex for avoiding race conditions while calculating the CRC
    static std::mutex crc_mtx;
public:
    /**
     * Constructor. Instantiates the first packet to send.
     * \return
     */
    XmodemPacket() : XmodemPacket(1) {};

    /**
     * Returns the empty structure of the next packet to send.
     * \return the next packet.
     */
    XmodemPacket next();

    /**
     * Populates the packet data using an input stream (usually a file).
     * \throws FileIOException when the read data does not contain all the requested bytes but the file did not end.
     * \return
     */
    void read_from_binfile(std::ifstream &file);

    /**
     * Computes and populates the CRC of the packet.
     * It uses a CRC16-CCIT variant with the remainder initialized to zero.
     * \return
     */
    void compute_crc();

    /**
     * Gets the serialized packet ready to be sent over XMODEM.
     * \return The serialized packet.
     */
    char *get_content();
};


#endif //WANDSTEM_FLASH_UTILITY_XMODEMPACKET_H
