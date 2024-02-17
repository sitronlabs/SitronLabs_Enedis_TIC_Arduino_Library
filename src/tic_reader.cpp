/* Self header */
#include "tic_reader.h"

#ifndef CONFIG_TIC_DEBUG_ENABLED
#define CONFIG_TIC_DEBUG_ENABLED 0  //!< Set this to 1 to turn on verbose debugging.
#endif
#if CONFIG_TIC_DEBUG_ENABLED
#ifndef CONFIG_TIC_DEBUG_FUNCTION
#define CONFIG_TIC_DEBUG_FUNCTION(x) Serial.println(F(x))
#endif
#else
#define CONFIG_TIC_DEBUG_FUNCTION(x)
#endif

/**
 * @brief
 * @param[in] uart
 * @return
 */
int tic_reader::setup(Stream &uart) {

    /* Save uart */
    m_stream = &uart;

    /* Reset state machine */
    m_sm = STATE_0;

    /* Return success */
    return 0;
}

/**
 * @brief
 * @param[out] dataset
 * @return
 */
int tic_reader::read(struct tic_dataset &dataset) {

    /* Ensure setup has been performed */
    if (m_stream == NULL) {
        return -EINVAL;
    }

    /* Process incoming bytes */
    while (m_stream->available() > 0) {

        /* Read byte */
        int rx = m_stream->read();
        if (rx < 0 || rx > UINT8_MAX) {
            CONFIG_TIC_DEBUG_FUNCTION(" [e] Read error!");
            m_sm = STATE_0;
            return -EIO;
        }

        /* Perform parity check */
        if (__builtin_parity(rx) != 0) {
            CONFIG_TIC_DEBUG_FUNCTION(" [e] Parity error!");
            m_sm = STATE_0;
            return -EIO;
        }

        /* Remove parity bit */
        rx = rx & 0x7F;

        /* Reception state machine */
        switch (m_sm) {

            case STATE_0: {  // Wait for frame start

                if (rx == 0x02) {
                    m_sm = STATE_1;
                    break;
                }
                break;
            }

            case STATE_1: {  // Wait for either dataset start or frame stop

                if (rx == 0x0A) {  // Dataset start
                    m_dataset_buffer_index = 0;
                    m_sm = STATE_2;
                } else if (rx == 0x03) {  // Frame stop
                    m_sm = STATE_0;
                } else {
                    m_sm = STATE_0;
                    return -EIO;
                }
                break;
            }

            case STATE_2: {  // Read dataset name

                /* If we have received one of the two possible splitter chars,
                 * take note of the type of splitter char as a way to differentiate between historic and standard datasets,
                 * move on to processing the other parts of the dataset */
                if (rx == 0x09 || rx == 0x20) {
                    m_dataset_buffer[m_dataset_buffer_index] = rx;
                    m_dataset_buffer_index++;
                    m_splitter_char = (char)rx;
                    m_sm = STATE_3;
                }

                /* Otherwise, keep appending */
                else {
                    if (m_dataset_buffer_index >= TIC_PARSER_DATASET_NAME_LENGTH_MAX) {
                        CONFIG_TIC_DEBUG_FUNCTION(" [e] Dataset name too long!");
                        m_sm = STATE_0;
                        return -EIO;
                    } else {
                        m_dataset_buffer[m_dataset_buffer_index] = rx;
                        m_dataset_buffer_index++;
                    }
                }
                break;
            }

            case STATE_3: {  // Read dataset time and data

                /* If we have received the end of dataset character */
                if (rx == 0x0D) {

                    /* Verify checksum:
                     *  - for historic datasets, where the splitter char is 0x20, the checksum doesn't include the last splitter char,
                     *  - for standard datasets, where the splitter char is 0x09, the checkum includes the last splitter char.
                     * Go figure */
                    if (m_splitter_char == 0x20) {
                        uint8_t checksum_received = m_dataset_buffer[m_dataset_buffer_index - 1];
                        uint8_t checksum_computed = 0x00;
                        for (uint8_t i = 0; i < m_dataset_buffer_index - 2; i++) {
                            checksum_computed += m_dataset_buffer[i];
                        }
                        checksum_computed = (checksum_computed & 0x3F) + 0x20;
                        if (checksum_computed != checksum_received) {
                            CONFIG_TIC_DEBUG_FUNCTION(" [e] Corrupt dataset!");
                            m_sm = STATE_0;
                            return -EIO;
                        }
                    } else if (m_splitter_char == 0x09) {
                        uint8_t checksum_received = m_dataset_buffer[m_dataset_buffer_index - 1];
                        uint8_t checksum_computed = 0x00;
                        for (uint8_t i = 0; i < m_dataset_buffer_index - 1; i++) {
                            checksum_computed += m_dataset_buffer[i];
                        }
                        checksum_computed = (checksum_computed & 0x3F) + 0x20;
                        if (checksum_computed != checksum_received) {
                            CONFIG_TIC_DEBUG_FUNCTION(" [e] Corrupt dataset!");
                            m_sm = STATE_0;
                            return -EIO;
                        }
                    } else {
                        CONFIG_TIC_DEBUG_FUNCTION(" [e] Unsupported splitter char!");
                        m_sm = STATE_0;
                        return -EIO;
                    }

                    /* Count number of splitters in buffer,
                     * and while we are at it, replace splitters by null chars to make string processing easier */
                    uint8_t splitter_posistions[3] = {0};
                    uint8_t splitter_count = 0;
                    for (uint8_t i = 0; i < m_dataset_buffer_index - 1; i++) {
                        if (m_dataset_buffer[i] == m_splitter_char) {
                            if (splitter_count < 3) {
                                splitter_posistions[splitter_count] = i;
                                m_dataset_buffer[i] = '\0';
                            }
                            splitter_count++;
                        }
                    }

                    /* Fill dataset struct and move on */
                    if (splitter_count == 2) {
                        uint8_t name_pos = 0;
                        uint8_t data_pos = splitter_posistions[0] + 1;
                        uint8_t name_length = min(TIC_PARSER_DATASET_NAME_LENGTH_MAX, splitter_posistions[0]);
                        uint8_t data_length = min(TIC_PARSER_DATASET_DATA_LENGTH_MAX, splitter_posistions[1] - (splitter_posistions[0] + 1));
                        strncpy(dataset.name, (char *)(&m_dataset_buffer[name_pos]), name_length);
                        strncpy(dataset.data, (char *)(&m_dataset_buffer[data_pos]), data_length);
                        dataset.name[name_length] = '\0';
                        dataset.time[0] = '\0';
                        dataset.data[data_length] = '\0';
                        m_sm = STATE_1;
                        return 1;
                    } else if (splitter_count == 3) {
                        uint8_t name_pos = 0;
                        uint8_t time_pos = splitter_posistions[0] + 1;
                        uint8_t data_pos = splitter_posistions[1] + 1;
                        uint8_t name_length = min(TIC_PARSER_DATASET_NAME_LENGTH_MAX, splitter_posistions[0]);
                        uint8_t time_length = min(TIC_PARSER_DATASET_TIME_LENGTH_MAX, splitter_posistions[1] - (splitter_posistions[0] + 1));
                        uint8_t data_length = min(TIC_PARSER_DATASET_DATA_LENGTH_MAX, splitter_posistions[2] - (splitter_posistions[1] + 1));
                        strncpy(dataset.name, (char *)(&m_dataset_buffer[name_pos]), name_length);
                        strncpy(dataset.time, (char *)(&m_dataset_buffer[time_pos]), time_length);
                        strncpy(dataset.data, (char *)(&m_dataset_buffer[data_pos]), data_length);
                        dataset.name[name_length] = '\0';
                        dataset.time[time_length] = '\0';
                        dataset.data[data_length] = '\0';
                        m_sm = STATE_1;
                        return 1;
                    } else {
                        CONFIG_TIC_DEBUG_FUNCTION(" [e] Invalid splitter count!");
                        m_sm = STATE_0;
                        return -EIO;
                    }
                    break;
                }

                /* Otherwise, keep appending data to current dataset */
                else {
                    if (m_dataset_buffer_index >= (TIC_PARSER_DATASET_NAME_LENGTH_MAX + 1 + TIC_PARSER_DATASET_TIME_LENGTH_MAX + 1 + TIC_PARSER_DATASET_DATA_LENGTH_MAX + 1 + TIC_PARSER_DATASET_CHECKSUM_LENGTH_MAX)) {
                        CONFIG_TIC_DEBUG_FUNCTION(" [e] Dataset content too long!");
                        m_sm = STATE_0;
                    } else {
                        m_dataset_buffer[m_dataset_buffer_index] = rx;
                        m_dataset_buffer_index++;
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

    /* Return no dataset */
    return 0;
}
