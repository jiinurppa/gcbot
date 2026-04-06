#include "hw_config.h"
#include "f_util.h"
#include "ff.h"
#include "dtm_writer.hpp"
#include <cstdint>
#include <cstring>
#include <string>

void DTMWriter::init()
{
    FRESULT fr = f_mount(&_fs, "", 1);

    if (fr != FR_OK)
    {
        _error_message = "MOUNT ERROR";
        return;
    }

    auto filename = "REC.DTM";

    fr = f_open(&_fil, filename, FA_CREATE_ALWAYS | FA_WRITE);

    if (fr != FR_OK)
    {
        _error_message = "FILE CREATE ERROR";
        return;
    }

    // Fill header with zeroes
    f_lseek(&_fil, 0);
    uint8_t buf[1] = { 0 };
    UINT bytes_written = 0;
    for (size_t i = 0; i < INPUT_START_POS; ++i)
    {
        f_write(&_fil, buf, 1, &bytes_written);
    }

    // Write DTM signature
    f_lseek(&_fil, 0);
    uint8_t signature[4] = { 0x44, 0x54, 0x4D, 0x1A };
    f_write(&_fil, signature, 4, &bytes_written);

    if (fr != FR_OK || bytes_written != 4)
    {
        _error_message = "SIGN. ERROR";
        return;
    }

    // Write number of controllers (1)
    buf[0] = 1;
    f_lseek(&_fil, CONTROLLERS_POS);
    f_write(&_fil, buf, 1, &bytes_written);

    if (fr != FR_OK || bytes_written != 1)
    {
        _error_message = "CONT. ERROR";
        return;
    }

    // Sync and seek to first input position
    f_sync(&_fil);
    f_lseek(&_fil, INPUT_START_POS);
    _file_open = true;
}

DTMWriter::DTMWriter()
{
    init();
}

DTMWriter::~DTMWriter()
{
    if (_file_was_finalized || !_file_open)
    {
        return;
    }

    // File will not be healthy
    f_close(&_fil);
    f_unmount("");
}

bool DTMWriter::write_input(const uint8_t *bytes)
{
    if (!_file_open)
    {
        _error_message = "WRITE:FILE CLOSED";
        return false;
    }

    if (_file_was_finalized)
    {
        _error_message = "WRITE:FILE FINALIZED";
        return false;
    }

    UINT bytes_written = 0;
    FRESULT fr = f_write(&_fil, bytes, INPUT_SIZE_BYTES, &bytes_written);

    if (fr != FR_OK || bytes_written != INPUT_SIZE_BYTES)
    {
        _error_message = "WRITE ERROR";
        return false;
    }

    _sector_byte_counter += bytes_written;

    // Sync when sector size is reached
    if (_sector_byte_counter >= SECTOR_SIZE)
    {
        _sector_byte_counter = 0;
        fr = f_sync(&_fil);

        if (fr != FR_OK)
        {
            _error_message = "WRIITE SYNC ERROR";
            return false;
        }
    }

    ++_inputs_written;

    return true;
}

bool DTMWriter::finalize_file()
{
    if (!_file_open)
    {
        _error_message = "FINALIZE: FILE CLOSED";
        return false;
    }

    if (_file_was_finalized)
    {
        return true;
    }

    // Write inputs that haven't been synced
    FRESULT fr = f_sync(&_fil);

    if (fr != FR_OK)
    {
        _error_message = "FINALIZE SYNC ERROR";
        return false;
    }

    // Write input count to DTM header
    uint8_t inputs[8];

    for (size_t i = 0; i < 8; ++i)
    {
        inputs[i] = uint8_t((_inputs_written >> (8 * i)) & 0xFF);
    }

    fr = f_lseek(&_fil, INPUT_COUNT_POS);

    if (fr != FR_OK)
    {
        _error_message = "FINALIZE SEEK ERROR";
        return false;
    }

    UINT bytes_written = 0;
    f_write(&_fil, inputs, 8, &bytes_written);

    if (fr != FR_OK || bytes_written != 8)
    {
        _error_message = "IN. COUNT ERROR";
        return false;
    }

    // f_close also syncs
    fr = f_close(&_fil);

    if (fr != FR_OK)
    {
        _error_message = "CLOSE ERROR";
        return false;
    }

    _file_open = false;

    fr = f_unmount("");

    if (fr != FR_OK)
    {
        _error_message = "UNMOUNT ERROR";
        return false;
    }

    _file_was_finalized = true;
    return true;
}
