/* Self header */
#include "tic.h"

/**
 * @brief
 * @param[in] uart
 * @return
 */
int tic::setup(Stream &uart) {

    /* Save uart */
    m_stream = &uart;

    /* Reset state machine */
    m_sm = STATE_0;

    /* Return success */
    return 0;
}

/**
 * @brief
 * @param[out] message
 * @return
 */
int tic::process(struct tic_message &message) {

    /* Ensure setup has been performed */
    if (m_stream == NULL) {
        return -EINVAL;
    }

    /* Process incoming bytes */
    while (m_stream->available() > 0) {

        /* Read byte */
        int rx = m_stream->read();
        if (rx < 0 || rx > UINT8_MAX) {
            m_sm = STATE_0;
            return -EIO;
        }

        /* Perform parity check */
        if (__builtin_parity(rx) != 0) {
            m_sm = STATE_0;
            return -EIO;
        }

        /* Remove parity bit */
        rx = rx & 0x7F;

        /* Parsing state machine */
        switch (m_sm) {

            case STATE_0: {  // Wait for frame start

                if (rx == 0x02) {
                    m_sm = STATE_1;
                    break;
                }
                break;
            }

            case STATE_1: {  // Wait for either message start or frame stop

                if (rx == 0x0A) {  // Message start
                    m_buffer_index = 0;
                    m_sm = STATE_2;
                } else if (rx == 0x03) {  // Frame stop
                    m_sm = STATE_0;
                } else {
                    m_sm = STATE_0;
                    return -EIO;
                }
                break;
            }

            case STATE_2: {  // Parse message tag

                if (rx == 0x09 || rx == 0x20) {
                    m_buffer[m_buffer_index] = rx;
                    m_buffer_index++;
                    m_splitter_char = rx;
                    m_sm = STATE_3;
                } else {
                    if (m_buffer_index >= 8) {
                        m_sm = STATE_0;
                        return -EIO;
                    } else {
                        m_buffer[m_buffer_index] = rx;
                        m_buffer_index++;
                    }
                }
                break;
            }

            case STATE_3: {  // Wait for message stop

                /* If we receive the end of message character */
                if (rx == 0x0D) {

                    /* Verify checksum:
                     *  - for historic messages, where the splitter char is 0x20, the checksum doesn't include the last splitter char,
                     *  - for standard messages, where the splitter char is 0x09, the checkum includes the last splitter char.
                     * Go figure */
                    if (m_splitter_char == 0x20) {
                        uint8_t checksum_received = m_buffer[m_buffer_index - 1];
                        uint8_t checksum_computed = 0x00;
                        for (size_t i = 0; i < m_buffer_index - 2; i++) checksum_computed += m_buffer[i];
                        checksum_computed = (checksum_computed & 0x3F) + 0x20;
                        if (checksum_computed != checksum_received) {
                            m_sm = STATE_0;
                            return -EIO;
                        }
                    } else if (m_splitter_char == 0x09) {
                        uint8_t checksum_received = m_buffer[m_buffer_index - 1];
                        uint8_t checksum_computed = 0x00;
                        for (size_t i = 0; i < m_buffer_index - 1; i++) checksum_computed += m_buffer[i];
                        checksum_computed = (checksum_computed & 0x3F) + 0x20;
                        if (checksum_computed != checksum_received) {
                            m_sm = STATE_0;
                            return -EIO;
                        }
                    } else {
                        m_sm = STATE_0;
                        return -EIO;
                    }

                    /* Split message */
                    uint8_t splitter_posistions[3] = {0};
                    uint8_t splitter_count = 0;
                    for (uint8_t i = 0; i < m_buffer_index - 1; i++) {
                        if (m_buffer[i] == m_splitter_char) {
                            if (splitter_count < 3) {
                                splitter_posistions[splitter_count] = i;
                            }
                            splitter_count++;
                        }
                    }

                    /* Fill message struct and move on */
                    if (splitter_count == 2) {
                        strncpy(message.name, (char *)(&m_buffer[0]), splitter_posistions[0]);
                        strncpy(message.data, (char *)(&m_buffer[splitter_posistions[0] + 1]), splitter_posistions[1] - (splitter_posistions[0] + 1));
                        message.name[min(8, splitter_posistions[0])] = '\0';
                        message.data[min(12, splitter_posistions[1] - (splitter_posistions[0] + 1))] = '\0';
                        m_sm = STATE_1;
                        return 1;
                    } else if (splitter_count == 3) {
                        strncpy(message.name, (char *)(&m_buffer[0]), splitter_posistions[0]);
                        strncpy(message.data, (char *)(&m_buffer[splitter_posistions[1] + 1]), splitter_posistions[2] - (splitter_posistions[1] + 1));
                        message.name[min(8, splitter_posistions[0])] = '\0';
                        message.data[min(12, splitter_posistions[2] - (splitter_posistions[1] + 1))] = '\0';
                        m_sm = STATE_1;
                        return 1;
                    } else {
                        m_sm = STATE_0;
                        return -EIO;
                    }
                    break;
                }

                /* Otherwise, keep appending data to current message */
                else {
                    if (m_buffer_index >= 37) {
                        m_sm = STATE_0;
                    } else {
                        m_buffer[m_buffer_index] = rx;
                        m_buffer_index++;
                    }
                }
                break;
            }

            default: {
                m_sm = STATE_0;
                break;
            }
        }
    }

    /* Return no message */
    return 0;
}
