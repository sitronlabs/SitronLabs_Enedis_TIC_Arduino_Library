#ifndef TIC_READER_H
#define TIC_READER_H

/* Arduino libraries */
#include <Arduino.h>

/* C/C++ libraries */
#include <errno.h>
#include <stdint.h>

/* Config */
#define TIC_PARSER_DATASET_NAME_LENGTH_MAX 8
#define TIC_PARSER_DATASET_TIME_LENGTH_MAX 13
#define TIC_PARSER_DATASET_DATA_LENGTH_MAX 98
#define TIC_PARSER_DATASET_CHECKSUM_LENGTH_MAX 1

/**
 * Structure used to store a dataset when received.
 */
struct tic_dataset {
    char name[TIC_PARSER_DATASET_NAME_LENGTH_MAX + 1];  //!< Label of the dataset (null-terminated).
    char time[TIC_PARSER_DATASET_TIME_LENGTH_MAX + 1];  //!< Timestamp of the dataset (null-terminated).
    char data[TIC_PARSER_DATASET_DATA_LENGTH_MAX + 1];  //!< Value of the dataset (null-terminated).
};

/**
 * Class to read datasets from Linky meters.
 * Linky meters continously send frames of data, each frame composed of multiple datasets.
 * You can think of a dataset as variable: it has a name and a value.
 * An entire frame might be too big for the ram of some small microcontrollers, so this class allows to read datasets one by one.
 */
class tic_reader {
   public:
    /**
     * @brief Setup function.
     * @param[in] uart A reference to the serial port class that will be used to read incoming bytes.
     * @return 0 in case of success, or a negative error code otherwise.
     * @note The serial will have to be initialized first with the correct baudrate (1200 for historic, 9600 for standard).
     */
    int setup(Stream &uart);

    /**
     * @brief Processes incoming data.
     * @param[out] data A reference to a structure that will be filled when a complete dataset has been received.
     * @return 1 when a complete dataset has been received, 0 when it still needs to be called, or a negative error code otherwise.
     * @note Should be called regularly to treat bytes as they are received.
     */
    int read(struct tic_dataset &dataset);

   protected:
    Stream *m_stream = NULL;
    enum {
        STATE_0,
        STATE_1,
        STATE_2,
        STATE_3,
        STATE_ERROR,
    } m_sm;
    char m_splitter_char;
    char m_dataset_buffer[TIC_PARSER_DATASET_NAME_LENGTH_MAX + 1 + TIC_PARSER_DATASET_TIME_LENGTH_MAX + 1 + TIC_PARSER_DATASET_DATA_LENGTH_MAX + 1 + TIC_PARSER_DATASET_CHECKSUM_LENGTH_MAX];
    uint8_t m_dataset_buffer_index;
};

#endif
