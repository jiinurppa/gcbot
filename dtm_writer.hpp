#ifndef DTM_WRITER_HPP
#define DTM_WRITER_HPP

#include "ff.h"
#include <cstdint>
#include <string>

class DTMWriter
{
private:
    static constexpr FSIZE_t CONTROLLERS_POS = 0x00B;
    static constexpr FSIZE_t INPUT_COUNT_POS = 0x015;
    static constexpr FSIZE_t INPUT_START_POS = 0x100;
    static constexpr uint16_t SECTOR_SIZE = 512;

    bool _file_open = false;
    bool _file_was_finalized = false;
    uint16_t _sector_byte_counter = 0;
    uint64_t _inputs_written = 0;
    std::string _error_message = "";
    FATFS _fs;
    FIL _fil;

    void init();

public:
    static constexpr size_t INPUT_SIZE_BYTES = 8;

    DTMWriter();
    ~DTMWriter();
    uint64_t inputs_written() const { return _inputs_written; }
    bool write_input(const uint8_t* bytes);
    bool finalize_file();
    const std::string& error_message() const { return _error_message; }
};

#endif /* DTM_WRITER_HPP */