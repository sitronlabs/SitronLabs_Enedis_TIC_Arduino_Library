# Enedis TIC Arduino Library
Arduino library for reading data from Linky electricity meters in France

### Related products
| <a href="https://www.tindie.com/products/31871/"><img src="https://cdn.tindiemedia.com/images/resize/9G3L9AM5skGMUX2PxAac2NLPNAI=/p/fit-in/653x435/filters:fill(fff)/i/176541/products/2023-12-28T19%3A48%3A47.192Z-1.jpg" alt="Enedis TIC Arduino Shield" width="200" height="auto" /></a> | <a href="https://www.tindie.com/products/29954/"><img src="https://cdn.tindiemedia.com/images/resize/L-6lGeZyBsQGWax18Ktj7XyD5DM=/p/fit-in/653x435/filters:fill(fff)/i/176541/products/2023-12-28T16%3A05%3A22.618Z-IMG_1541.jpg" alt="MySensors nRF24 Linky Module" width="200" height="auto"></a> | 
|:--:|:--:|
| *<a href="https://www.tindie.com/products/31871/">Enedis TIC Arduino Shield</a>* | *<a href="https://www.tindie.com/products/29954/">MySensors nRF24 Linky Module</a>* |

### Description
Electricity meters in France expose real time information through the consumer-side Télé-Information Client (TIC) output. The output is similar to a serial port. This library allows to process and decode the received bytes.

### Features
* Supports different serial ports
* Low ram consumption by processing data on the fly
* Computes bit parity and CRC

### Usage
The `tic_reader` class has a `setup()` function which expects a serial port in the form of a reference to a `HardwareSerial` instance. That serial port should already be initialized with a baudrate of either `1200` for historic mode, or `9600` for standard.

Once setup has been called, you should call the `read()` function periodically to process incoming data. When enough bytes have been received, the `read()` function will return `1`, and the `dataset` argument will be filled with a received key-value pair. All possible key-value pairs are described in section 6 of the official [Enedis NOI-CPT_54E.pdf](https://github.com/sitronlabs/SitronLabs_Enedis_TIC_Arduino_Library/blob/master/doc/Enedis%20NOI-CPT_54E.pdf) document.

For more information, have a look a the [example firmware](https://github.com/sitronlabs/SitronLabs_Enedis_TIC_Arduino_Shield_Example/blob/master/src/main.cpp).
