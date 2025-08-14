//Class created for interfacing with MAX31865 chip connected with a Pt100 or Pt1000 temperature probe
//Simone Valdre', 29/07/2025 (created)

#include "maxlib.h"

MaxRTDProbe::MaxRTDProbe(const int &dev, const int &ce, const uint8_t mode) {
    char devname[20];

    fZombie = true;
    sprintf(devname, "/dev/spidev%1d.%1d", dev, ce);
    
    // Open SPI device
    if((fd = open(devname, O_RDWR)) < 0) {
        perror(devname);
        return;
    }

    // Set SPI mode
	if(ioctl(fd, SPI_IOC_WR_MODE, &mode) <0) {
	    perror("SPI_IOC_WR_MODE");
        close(fd);
        return;
	}

    //Setting transmission structure (custom values can be set via specific functions)
    tr.bits_per_word = BITS;
    tr.speed_hz      = SPEED;
    tr.delay_usecs   = DELAY;
    tr.cs_change     = CSCHANGE;
    tr.rx_buf        = (uint64_t)rx;
    tr.tx_buf        = (uint64_t)tx;

    sConf = 0;
    Rcal = RCAL;

    fZombie = false;
}

MaxRTDProbe::~MaxRTDProbe() {
    if(!fZombie) {
        close(fd);
    }
    printf("DEBUG: closing\n");
}

bool MaxRTDProbe::IsZombie() {
    return fZombie;
}

void MaxRTDProbe::SetBits(const uint8_t &bits) {
    tr.bits_per_word = bits;
}

void MaxRTDProbe::SetSpeed(const uint32_t &speed) {
    tr.speed_hz      = speed;
}

void MaxRTDProbe::SetDelay(const uint16_t &delay) {
    tr.delay_usecs   = delay;
}

void MaxRTDProbe::SetCSchange(const uint8_t &cschange) {
    tr.cs_change     = cschange;
}

int MaxRTDProbe::WriteRegister(const uint8_t &reg, const std::vector < uint8_t > &data) {
    if(fZombie) return -1;
    if(data.size() + 1 > BUFSIZE) {
        printf("WriteRegister: trying to write more than allowed data\n");
        return -1;
    }

    tr.len = data.size() + 1;
    tx[0] = (reg|0x80);
    for(size_t i = 0; i < data.size(); i++) {
        tx[i+1] = data[i];
    }
    // Read and write data (full duplex)
    if(ioctl(fd, SPI_IOC_MESSAGE(1), &tr) < 0) {
        perror("WriteRegister");
        return -1;
    }
    return 0;
}

int MaxRTDProbe::ReadRegister(const uint8_t &reg, std::vector < uint8_t > &data) {
    if(fZombie) return -1;
    if(data.size() + 1 > BUFSIZE) {
        printf("ReadRegister: trying to read more than allowed data\n");
        return -1;
    }

    tr.len = data.size() + 1;
    tx[0] = reg;
    for(size_t i = 0; i < data.size(); i++) {
        tx[i+1] = data[i];
    }
    // Read and write data (full duplex)
    if(ioctl(fd, SPI_IOC_MESSAGE(1), &tr) < 0) {
        perror("ReadRegister");
        return -1;
    }
    for(size_t i = 0; i < data.size(); i++) {
        data[i] = rx[i+1];
    }
    return 0;
}

int MaxRTDProbe::Configure(const uint8_t &config) {
    if(fZombie) return -1;
    std::vector < uint8_t > data(1,0);
    data[0] = config;

    if(WriteRegister(0x00, data) < 0) return -1;
    if(ReadRegister(0x00, data) < 0) return -1;

    if(data[0] != (config&221)) {
        printf("Configure: configuration failed - return value %02x\n", data[0]);
        return -1;
    }
    return 0;
}

int MaxRTDProbe::Configure(const bool &Vbias, const bool &mode, const bool &wire3, const bool &hz50) {
    if(fZombie) return -1;
    uint8_t config = 0;
    if(Vbias) config += 128;
    if(mode)  config +=  64;
    if(wire3) config +=  16;
    if(hz50)  config +=   1;
    //storing configuration data
    sConf = config;

    //always reset faults when reconfiguring:
    config +=2;
    return Configure(config);
}

int MaxRTDProbe::ResetFaultStatus() {
    if(fZombie) return -1;
    uint8_t config = (sConf|2);
    return Configure(config);
}

int MaxRTDProbe::FaultDetectionCycle() {
    if(fZombie) return -1;
    std::vector < uint8_t > data(1,0);
    data[0] = ((sConf&17)|4);

    if(WriteRegister(0x00, data) < 0) return -1;
    for(int i = 0; i < 10; i++) {
        usleep(1000);
        if(ReadRegister(0x00, data) < 0) return -1;
        if((data[0]&12) == 0) break;
        if(i == 9) printf("FaultDetectionCycle: not ended after 10 ms!\n");
    }
    
    if(ReadRegister(0x07, data) < 0) return -1;
    ResetFaultStatus();
    if((data[0]&252) == 0) return 0;
    if(data[0]&128) printf(YEL "Fault detected:" NRM "RTD High Threshold\n");
    if(data[0]& 64) printf(YEL "Fault detected:" NRM "RTD Low Threshold\n");
    if(data[0]& 32) printf(YEL "Fault detected:" NRM "REFIN- > 0.85 * Vbias\n");
    if(data[0]& 16) printf(YEL "Fault detected:" NRM "REFIN- < 0.85 * Vbias (FORCE- open)\n");
    if(data[0]&  8) printf(YEL "Fault detected:" NRM "RTDIN- < 0.85 * Vbias (FORCE- open)\n");
    if(data[0]&  4) printf(YEL "Fault detected:" NRM "overvoltage / undervoltage\n");
    return -1;
}

int MaxRTDProbe::ADCRead(double &adc) {
    if(fZombie) return -1;
    std::vector < uint8_t > data(2,0);
    if(ReadRegister(0x01, data) < 0) return -1;
    adc = (double)((((int)(data[0])) << 7) + (((int)(data[1])) >> 1));
    return (data[1]%2);
}

int MaxRTDProbe::SetRcal(const double &_Rcal) {
    if(Rcal > 1.e-3 || Rcal < 0) {
        printf(YEL "SetRcal" NRM ": bad Rcal value provided!\n");
        return -1;
    }
    Rcal = _Rcal;
    return 0;
}

int MaxRTDProbe::SetRcal(const double &Rref, const double &R0) {
    return SetRcal(Rref / (R0 * 32768.));
}

double MaxRTDProbe::Tconv(const double &adc) {
    printf("\nDEBUG: Rcal = %e", Rcal);
    double rr = adc * Rcal;
    if(rr > 500) return 1000; //Too high temperature
    //Quadratic approximation valid from 0 to 850 C (rr==1 means 0 C)
    double T = (3.9083e-3 - sqrt(1.52748e-5 - 2.31e-6 * (rr - 1))) *  865801.;
    if(rr >= 1) return T;
    //iterative procedure under 0 C
    double Tn, R, DR;
    for(int i = 0; i < NSTEPS; i++) {
        Tn = T;
		R  = 1. + a * T + b * T * T + c * (T - 100.) * T * T * T;
		DR = a + 2 * b * T + 4 * c * (T - 75.) * T * T;
		T = Tn + (rr - R) / DR;
		if(fabs(Tn - T) <= TPREC) break;
        if(i == NSTEPS - 1) printf(YEL "Tconv" NRM ": maximum number of steps reached with residual precision of %f\n", fabs(Tn - T));
	}
    return T;
}
