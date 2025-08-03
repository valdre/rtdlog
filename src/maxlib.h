//Class created for interfacing with MAX31865 chip connected with a Pt100 or Pt1000 temperature probe
//Simone Valdre', 29/07/2025 (created)

#ifndef __MAXLIB
#define __MAXLIB

#include <cstdio>
#include <cmath>
#include <vector>
#include <fcntl.h>
#include <unistd.h>
#include <inttypes.h>
#include <sys/ioctl.h>
#include <linux/spi/spidev.h>
#include "ShellColors.h"

//Default SPI communication parameters (ideal values for MAX31865, you don't need to change them)
//Bits per word: 8 or 9
#define BITS       8
//Clock frequency [Hz]: max 5MHz for the RPi, max 1MHz for the MAX31865
#define SPEED 500000
//Time between end of data and CS de-assert
#define DELAY      0
//If CSCHANGE is not zero, it disables CS signal between one trasmission and the next one in multi-message transfers
#define CSCHANGE   0
//Default buffer size, 10 is more than sufficient!
#define BUFSIZE   10
//Default calibration factor = Rref / (R0 * 32768), assuming Rref = 400 Ohm and R0 = 100 Ohm
#define RCAL       0.000122
//temperature precision under 0 C
#define TPREC      0.01
//maximum number of steps for iterative conversion
#define NSTEPS    10


class MaxRTDProbe {
    private:
    struct spi_ioc_transfer tr;
    uint8_t tx[BUFSIZE], rx[BUFSIZE], sConf;
    int fd;
    bool fZombie;
    double Rcal;
    const double a = 3.9083e-3, b = -5.775e-7, c = -4.18301e-12; //temperature conversion coefficients
    //Single or multiple register writing. Specify the first register address
    int WriteRegister(const uint8_t &reg, const std::vector < uint8_t > &data);
    //Single or multiple register reading. Specify the first register address
    int ReadRegister(const uint8_t &reg, std::vector < uint8_t > &data);

    public:
    //dev and cs identify the SPI pins on Raspberry Pi, by default only device 0 is enabled,
    //CE 0 and 1 could be used with SPI0. Device name is "/dev/spidev<dev>.<cs>"
    //Pinout: SPI0 -> (SCLK: 23; MISO: 21; MOSI: 19; CE0: 24; CE1: 26)
    //Pinout: SPI1 -> (SCLK: 40; MISO: 35; MOSI: 38; CE0: 12; CE1: 11; CE2: 36)
    //PINS are not GPIO numbers!!!
    //MAX31865 supports SPI mode 1 and 3 only
    MaxRTDProbe(const int &dev = 0, const int &ce = 0, const uint8_t mode = SPI_MODE_1);
    ~MaxRTDProbe();
    bool IsZombie(); //if an error occurred while initializing the device
    void SetBits(const uint8_t &bits);
    void SetSpeed(const uint32_t &speed);
    void SetDelay(const uint16_t &delay);
    void SetCSchange(const uint8_t &cschange);
    //Chip configuration:
    //      Vbias = ON (true) / OFF (false)
    //      mode  = auto (true) / manual (false) sampling
    //      wire3 = 3-wire (true) / 2- or 4-wire (false) probe
    //      hz50  = 50Hz (true) / 60Hz (false) filtering
    int Configure(const bool &Vbias, const bool &mode, const bool &wire3 = true, const bool &hz50 = true);
    int Configure(const uint8_t &config);
    int ResetFaultStatus();
    int FaultDetectionCycle();
    int ADCRead(double &adc);

    int SetRcal(const double &_Rcal);
    int SetRcal(const double &Rref, const double &R0);
    double Tconv(const double &adc);
};

#endif
