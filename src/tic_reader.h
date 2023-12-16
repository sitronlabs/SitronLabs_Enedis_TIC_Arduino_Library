#ifndef TIC_READER_H
#define TIC_READER_H

/* Arduino libraries */
#include <Arduino.h>

/* C/C++ libraries */
#include <errno.h>
#include <stdint.h>

/**
 *
 */
struct tic_message {
    char name[8 + 1];   //!< Label of the dataset (null-terminated)
    char data[12 + 1];  //!< Content of the dataset (null-terminated)
};

/**
 *
 */
class tic_reader {
   public:
    /* Setup */
    int setup(Stream &uart);

    /* Process incoming data */
    int process(struct tic_message &message);

   protected:
    Stream *m_stream = NULL;
    enum {
        STATE_0,
        STATE_1,
        STATE_2,
        STATE_3,
        STATE_ERROR,
    } m_sm;
    size_t m_buffer_index;
    uint8_t m_buffer[8 + 1 + 13 + 1 + 12 + 1 + 1];
    uint8_t m_splitter_char;
};

#endif
