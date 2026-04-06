#include "hw_config.h"
#include "f_util.h"
#include "ff.h"
#include "dtm_reader.hpp"
#include <cstdint>
#include <cstring>
#include <string>

void DTMReader::init()
{
    FRESULT fr = f_mount(&_fs, "", 1);

    if (fr != FR_OK)
    {
        _error_message = "MOUNT ERROR";
        return;
    }

    auto filename = "REC.DTM";

    fr = f_open(&_fil, filename, FA_OPEN_EXISTING | FA_READ);

    if (fr != FR_OK && fr != FR_EXIST)
    {
        _error_message = "FILE NOT FOUND";
        return;
    }

    fr = f_lseek(&_fil, INPUT_COUNT_POS);

    if (fr != FR_OK)
    {
        _error_message = "SEEK ERROR 0X015";
        return;
    }

    uint8_t byte_read = 0;
    uint bytes_read = 0;

    for (auto i = 0; i < INPUT_SIZE_BYTES; ++i)
    {
        fr = f_read(&_fil, &byte_read, 1, &bytes_read);
        _input_count |= (uint64_t(byte_read) << (8 * i));
    }

    _file_open = true;
    reset_file_pos();
}

DTMReader::DTMReader()
{
    init();
}

DTMReader::~DTMReader()
{
    close_file();
}

void DTMReader::reset_file_pos()
{
    if (!_file_open)
    {
        _error_message = "RESET:FILE NOT OPEN";
        return;
    }

    FRESULT fr = f_lseek(&_fil, INPUT_START_POS);

    if (fr != FR_OK)
    {
        _error_message = "SEEK ERROR 0X100";
    }

    _inputs_read = 0;
}

bool DTMReader::get_next_input(uint8_t* input_bytes)
{
    if (!_file_open)
    {
        _error_message = "GET:FILE NOT OPEN";
        return false;
    }

    uint bytes_read = 0;
    FRESULT fr = f_read(&_fil, input_bytes, INPUT_SIZE_BYTES, &bytes_read);

    if (fr != FR_OK)
    {
        _error_message = "READ INPUT ERROR " + std::to_string((uint8_t)fr);
        return false;
    }

    if (bytes_read != 8)
    {
        _error_message = "READ SIZE NOT 8";
        return false;
    }

    ++_inputs_read;

    return true;
}

bool DTMReader::close_file()
{
    if (!_file_open)
    {
        return true;
    }

    FRESULT fr = f_close(&_fil);

    if (fr != FR_OK)
    {
        _error_message = "CLOSE ERROR";
        return false;
    }

    fr = f_unmount("");

    if (fr != FR_OK)
    {
        _error_message = "UNMOUNT ERROR";
        return false;
    }

    _file_open = false;
    return true;
}
