#include "../include/gbrw.h"
#include "../include/cartridge.hpp"

#pragma mark - Cartridge
bool Cartridge::isConnected() {
    // Check if a cartridge is connected
    // Do so by checking each byte of the title
    // If all bytes are 0xFF, then no cartridge is present
    for (size_t i = 0x134; i <= 0x143; i++) {
        uint8_t titleByte;
        gb_read(i, &titleByte, 1);

        if (titleByte != 0xFF) {
            return true;
        }
    }

    return false;
}

uint8_t Cartridge::getROMRevision() {
    // Read the ROM revision from the cartridge
    return gb_read_byte(0x14C);
}

size_t Cartridge::getROMBankCount() {
    // Read the ROM bank count from the cartridge
    return 2 << (size_t) gb_read_byte(0x148);
}

size_t Cartridge::getROMSize() {
    // Read the ROM size from the cartridge
    return getROMBankCount() * 16 * 1024;
}

size_t Cartridge::getRAMBankCount() {
    // Read the RAM bank count from the cartridge
    switch (gb_read_byte(0x149)) {
        case 0x00:
            return 0;
        case 0x01:
            return 0;
        case 0x02:
            return 1;
        case 0x03:
            return 4;
        case 0x04:
            return 16;
        case 0x05:
            return 8;
        default:
            return 0;
    }
}

size_t Cartridge::getRAMSize() {
    // Read the RAM size from the cartridge
    return getRAMBankCount() * 8 * 1024;
}

size_t Cartridge::readTitle(char *buf, bool *isColor) {
    // Read the title of the cartridge
    size_t read = gb_read(0x134, (uint8_t *)buf, 16);
    buf[read] = '\0';

    *isColor = false;
    if (read == 16) {
        if (buf[15] == 0x80 || buf[15] == 0xC0) {
            buf[15] = '\0';
            *isColor = true;
        }
    }

    return read;
}

uint8_t Cartridge::getCartridgeTypeRaw() {
    return gb_read_byte(0x147);
}

CartridgeTypes Cartridge::getCartridgeType() {
    uint8_t raw = getCartridgeTypeRaw();
    switch (raw) {
        case 0x00:
            return RomOnly;

        case 0x01:
        case 0x02:
        case 0x03:
            return MBC1;

        case 0x0F:
        case 0x10:
        case 0x11:
        case 0x12:
        case 0x13:
            return MBC3;

        case 0xFC:
            return Camera;

        default:
            return Unknown;
    }
}

size_t Cartridge::readROM(uint8_t *buf, size_t size, size_t offset) {
    switch (getCartridgeType()) {
        case RomOnly:
            if (offset >= 0x8000)
                return 0;

            if (offset + size > 0x8000)
                size = 0x8000 - offset;
            
            return gb_read(offset, buf, size);

        case MBC1:
            return MBC1Cartridge::readROM(buf, size, offset);

        case MBC3:
        case Camera:
            return MBC3Cartridge::readROM(buf, size, offset);

        default:
            return 0;
    }
}

size_t Cartridge::readRAM(uint8_t *buf, size_t size, size_t offset) {
    switch (getCartridgeType()) {
        case MBC1:
            return MBC1Cartridge::readRAM(buf, size, offset);

        case MBC3:
        case Camera:
            return MBC3Cartridge::readRAM(buf, size, offset);

        default:
            return 0;
    }
}

size_t Cartridge::writeRAM(const uint8_t *buf, size_t size, size_t offset) {
    switch (getCartridgeType()) {
        case MBC1:
            return MBC1Cartridge::writeRAM(buf, size, offset);

        case MBC3:
        case Camera:
            return MBC3Cartridge::writeRAM(buf, size, offset);

        default:
            return 0;
    }
}

#pragma mark - MBC1Cartridge
size_t MBC1Cartridge::readROM(uint8_t *buf, size_t size, size_t offset) {
    size_t bankCount = Cartridge::getROMBankCount();

    // Switch into advanced banking mode
    gb_write_byte(0x6000, 0x1);

    size_t readSize = 0;
    for (size_t i = offset >> 14; i < bankCount; i++) {
        if (readSize >= size) {
            break;
        }

        gb_write_byte(0x2000, i & 0x1F);
        gb_write_byte(0x4000, i >> 5);

        size_t offsetInBank = offset & 0x3FFF;
        size_t toRead = (16 * 1024) - offsetInBank;
        if (size - readSize < toRead) {
            toRead = size - readSize;
        }

        switch (i) {
            case 0x00:
            case 0x20:
            case 0x40:
            case 0x60:
                gb_read(0x0 + offsetInBank, buf, toRead);
                break;

            default:
                gb_read(0x4000 + offsetInBank, buf, toRead);
                break;
        }

        readSize += toRead;
        buf += toRead;
        offset = 0;
    }

    // Reset bank selection
    gb_write_byte(0x2000, 0x0);
    gb_write_byte(0x4000, 0x0);

    // Switch back to simple banking mode
    gb_write_byte(0x6000, 0x0);

    return readSize;
}

size_t MBC1Cartridge::readRAM(uint8_t *buf, size_t size, size_t offset) {
    size_t ramBankCount = Cartridge::getRAMBankCount();

    if (ramBankCount == 0) {
        return 0;
    }

    // Switch to advanced banking mode
    gb_write_byte(0x6000, 0x1);

    // Enable RAM
    gb_write_byte(0x0000, 0x0A);

    size_t readSize = 0;
    for (size_t i = offset >> 13; i < ramBankCount; i++) {
        if (readSize >= size) {
            break;
        }

        gb_write_byte(0x4000, i);

        size_t offsetInBank = offset & 0x1FFF;
        size_t toRead = (8 * 1024) - offsetInBank;
        if (size - readSize < toRead) {
            toRead = size - readSize;
        }

        gb_read(0xA000 + offsetInBank, buf, toRead);

        readSize += toRead;
        buf += toRead;
        offset = 0;
    }

    // Disable RAM
    gb_write_byte(0x0000, 0x00);

    // Reset bank selection
    gb_write_byte(0x4000, 0x0);

    // Switch back to simple banking mode
    gb_write_byte(0x6000, 0x0);

    return readSize;
}

size_t MBC1Cartridge::writeRAM(const uint8_t *buf, size_t size, size_t offset) {
    size_t ramBankCount = Cartridge::getRAMBankCount();

    if (ramBankCount == 0) {
        return 0;
    }

    // Switch to advanced banking mode
    gb_write_byte(0x6000, 0x1);

    // Enable RAM
    gb_write_byte(0x0000, 0x0A);

    size_t writeSize = 0;
    for (size_t i = offset >> 13; i < ramBankCount; i++) {
        if (writeSize >= size) {
            break;
        }

        gb_write_byte(0x4000, i);

        size_t offsetInBank = offset & 0x1FFF;
        size_t toWrite = (8 * 1024) - offsetInBank;
        if (size - writeSize < toWrite) {
            toWrite = size - writeSize;
        }

        gb_write(0xA000 + offsetInBank, buf, toWrite);

        writeSize += toWrite;
        buf += toWrite;
        offset = 0;
    }

    // Disable RAM
    gb_write_byte(0x0000, 0x00);

    // Reset bank selection
    gb_write_byte(0x4000, 0x0);

    // Switch back to simple banking mode
    gb_write_byte(0x6000, 0x0);

    return writeSize;
}

#pragma mark - MBC3Cartridge
size_t MBC3Cartridge::readROM(uint8_t *buf, size_t size, size_t offset) {
    // Implementation specific to MBC3
    size_t bankCount = Cartridge::getROMBankCount();

    size_t readSize = 0;
    for (size_t i = offset >> 14; i < bankCount; i++) {
        if (readSize >= size) {
            break;
        }

        gb_write_byte(0x2000, i);

        size_t offsetInBank = offset & 0x3FFF;
        size_t toRead = (16 * 1024) - offsetInBank;
        if (size - readSize < toRead) {
            toRead = size - readSize;
        }

        if (__builtin_expect(i == 0, 0))
            gb_read(0x0 + offsetInBank, buf, toRead);
        else
            gb_read(0x4000 + offsetInBank, buf, toRead);

        readSize += toRead;
        buf += toRead;
        offset = 0;
    }

    // Reset bank selection
    gb_write_byte(0x2000, 0x0);

    return readSize;
}

size_t MBC3Cartridge::readRAM(uint8_t *buf, size_t size, size_t offset) {
    // Implementation specific to MBC3
    size_t ramBankCount = Cartridge::getRAMBankCount();

    if (ramBankCount == 0) {
        return 0;
    }

    // Enable RAM
    gb_write_byte(0x0000, 0x0A);

    size_t readSize = 0;
    for (size_t i = offset >> 13; i < ramBankCount; i++) {
        if (readSize >= size) {
            break;
        }

        gb_write_byte(0x4000, i);

        size_t offsetInBank = offset & 0x1FFF;
        size_t toRead = (8 * 1024) - offsetInBank;
        if (size - readSize < toRead) {
            toRead = size - readSize;
        }

        gb_read(0xA000 + offsetInBank, buf, toRead);

        readSize += toRead;
        buf += toRead;
        offset = 0;
    }

    // Disable RAM
    gb_write_byte(0x0000, 0x00);

    // Reset bank selection
    gb_write_byte(0x4000, 0x0);

    return readSize;
}

size_t MBC3Cartridge::writeRAM(const uint8_t *buf, size_t size, size_t offset) {
    // Implementation specific to MBC3
    size_t ramBankCount = Cartridge::getRAMBankCount();

    if (ramBankCount == 0) {
        return 0;
    }

    // Enable RAM
    gb_write_byte(0x0000, 0x0A);

    size_t writeSize = 0;
    for (size_t i = offset >> 13; i < ramBankCount; i++) {
        if (writeSize >= size) {
            break;
        }

        gb_write_byte(0x4000, i);

        size_t offsetInBank = offset & 0x1FFF;
        size_t toWrite = (8 * 1024) - offsetInBank;
        if (size - writeSize < toWrite) {
            toWrite = size - writeSize;
        }

        gb_write(0xA000 + offsetInBank, buf, toWrite);

        writeSize += toWrite;
        buf += toWrite;
        offset = 0;
    }

    // Disable RAM
    gb_write_byte(0x0000, 0x00);

    // Reset bank selection
    gb_write_byte(0x4000, 0x0);

    return writeSize;
}
