#ifndef DTM_READER_HPP
#define DTM_READER_HPP

#include "ff.h"
#include <cstdint>
#include <string>

class DTMReader
{
private:
    static constexpr FSIZE_t INPUT_COUNT_POS = 0x015;
    static constexpr FSIZE_t INPUT_START_POS = 0x100;

    bool _file_open = false;
    uint64_t _input_count = 0;
    uint64_t _inputs_read = 0;
    std::string _error_message = "";
    FATFS _fs;
    FIL _fil;

    void init();

public:
    static constexpr size_t INPUT_SIZE_BYTES = 8;

    DTMReader();
    ~DTMReader();
    void reset_file_pos();
    bool file_open() const { return _file_open; }
    uint64_t input_count() const { return _input_count; }
    uint64_t inputs_read() const { return _inputs_read; }
    bool get_next_input(uint8_t* input_bytes);
    bool close_file();
    bool all_inputs_read() const { return _inputs_read == _input_count; }
    const std::string& error_message() const { return _error_message; }
};

#endif /* DTM_READER_HPP */