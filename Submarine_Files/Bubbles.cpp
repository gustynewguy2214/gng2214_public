#include <iostream>
#include <string>

#include "Bubbles.h"

using namespace std;

void testFunction(){
    cout << "test function worked!" << endl;
}

void isReady(){
    string tmp;
    cout << "Does the team feel this product is ready? [Y/(Anything else)]: ";
    cin >> tmp;
    cout << endl;
    if(tmp == "Y" || tmp == "y") return;
    else exit(0);
}

#include "bcm2835.c"
#include <cstdio>
#include <iostream>
#include <cstdint>
#include <cmath>

#define MS5837_ADDR               0x76
#define MS5837_RESET              0x1E
#define MS5837_ADC_READ           0x00
#define MS5837_PROM_READ          0xA0
#define MS5837_CONVERT_D1_8192    0x4A
#define MS5837_CONVERT_D2_8192    0x5A

#define MS5837_CONVERT_D1_2048    0x46
#define MS5837_CONVERT_D2_2048    0x56

const float Pa = 100.0f;
const float bar = 0.001f;
const float mbar = 1.0f;

const uint8_t MS5837_30BA = 0;
const uint8_t MS5837_02BA = 1;

uint16_t C[8];
uint32_t D1, D2;
int32_t TEMP;
int32_t P;
uint8_t _model;

/**
 * Expose the sensor to the air and calculate the bias parameter based on
 * the original depth data,the purpose is to make the depth 0
 **/
int32_t offsetP;

char sendBuf[5],recvBuf[5];  //IIC send of recv buffer

/**
 * bcm2835 iic error code
 * 0x00  BCM2835_I2C_REASON_OK 	Success
 * 0x01 BCM2835_I2C_REASON_ERROR_NACK 	Received a NACK
 * 0x02 BCM2835_I2C_REASON_ERROR_CLKT 	Received Clock Stretch Timeout
 * 0x03 BCM2835_I2C_REASON_ERROR_DATA 	Not all data is sent / received
 **/
uint8_t errCode;

float fluidDensity;

using namespace std;

uint8_t crc4(uint16_t n_prom[]) {
	uint16_t n_rem = 0;

	n_prom[0] = ((n_prom[0]) & 0x0FFF);
	n_prom[7] = 0;

	for ( uint8_t i = 0 ; i < 16; i++ ) {
		if ( i%2 == 1 ) {
			n_rem ^= (uint16_t)((n_prom[i>>1]) & 0x00FF);
		} else {
			n_rem ^= (uint16_t)(n_prom[i>>1] >> 8);
		}
		for ( uint8_t n_bit = 8 ; n_bit > 0 ; n_bit-- ) {
			if ( n_rem & 0x8000 ) {
				n_rem = (n_rem << 1) ^ 0x3000;
			} else {
				n_rem = (n_rem << 1);
			}
		}
	}

	n_rem = ((n_rem >> 12) & 0x000F);

	return n_rem ^ 0x00;
}

void depth_calculate() {
	// Given C1-C6 and D1, D2, calculated TEMP and P
	// Do conversion first and then second order temp compensation

	int32_t dT = 0;
	int64_t SENS = 0;
	int64_t OFF = 0;
	int32_t SENSi = 0;
	int32_t OFFi = 0;
	int32_t Ti = 0;
	int64_t OFF2 = 0;
	int64_t SENS2 = 0;

	// Terms called
	dT = D2-uint32_t(C[5])*256l;
	if ( _model == MS5837_02BA ) {
		SENS = int64_t(C[1])*65536l+(int64_t(C[3])*dT)/128l;
		OFF = int64_t(C[2])*131072l+(int64_t(C[4])*dT)/64l;
		P = (D1*SENS/(2097152.0)-OFF)/(32768.0);
	} else {
		SENS = int64_t(C[1])*32768l+(int64_t(C[3])*dT)/256l;
		OFF = int64_t(C[2])*65536l+(int64_t(C[4])*dT)/128l;
		P = (D1*SENS/(2097152.0)-OFF)/(8192.0);
	}

	// Temp conversion
	TEMP = 2000l+int64_t(dT)*C[6]/8388608LL;

	//Second order compensation
	if ( _model == MS5837_02BA ) {
		if((TEMP/100)<20){         //Low temp
			Ti = (11*int64_t(dT)*int64_t(dT))/(34359738368LL);
			OFFi = (31*(TEMP-2000)*(TEMP-2000))/8;
			SENSi = (63*(TEMP-2000)*(TEMP-2000))/32;
		}
	} else {
		if((TEMP/100)<20){         //Low temp
			Ti = (3*int64_t(dT)*int64_t(dT))/(8589934592LL);
			OFFi = (3*(TEMP-2000)*(TEMP-2000))/2;
			SENSi = (5*(TEMP-2000)*(TEMP-2000))/8;
			if((TEMP/100)<-15){    //Very low temp
				OFFi = OFFi+7*(TEMP+1500l)*(TEMP+1500l);
				SENSi = SENSi+4*(TEMP+1500l)*(TEMP+1500l);
			}
		}
		else if((TEMP/100)>=20){    //High temp
			Ti = 2*(dT*dT)/(137438953472LL);
			OFFi = (1*(TEMP-2000)*(TEMP-2000))/16;
			SENSi = 0;
		}
	}

	OFF2 = OFF-OFFi;           //Calculate pressure and temp second order
	SENS2 = SENS-SENSi;

	if ( _model == MS5837_02BA ) {
		TEMP = (TEMP-Ti);
		P = (((D1*SENS2)/2097152.0-OFF2)/32768.0)/100.0;
	} else {
		TEMP = (TEMP-Ti);
		P = (((D1*SENS2)/2097152.0-OFF2)/8192.0)/float(10+offsetP);
	}
}

void depth_read() {

	/**
	 * It is possible to use bcm to operate multiple IIC devices in the project.
	 * Before reading and writing the device, you need to change the slave device address.
	 * If there is only one iic slave in the project, you can comment the following statement
	 **/
	bcm2835_i2c_setSlaveAddress(MS5837_ADDR);

	// Request D1 conversion
	// sendBuf[0] = MS5837_CONVERT_D1_8192;
	sendBuf[0] = MS5837_CONVERT_D1_2048;
	if((errCode = bcm2835_i2c_write(sendBuf,1)))
	{
		printf("bcm2835_i2c_write failed at %s:%d, errCode = 0x%x\n",__FILE__,__LINE__,errCode);
		return;
	}

	// bcm_2835_delay(20); // Max conversion time per datasheet
	bcm2835_delay(5);  // When OSR = 2048, wait MS5837 converts, need 5ms

	sendBuf[0] = MS5837_ADC_READ;
	if((errCode = bcm2835_i2c_write_read_rs(sendBuf,1,recvBuf,3)))
	{
		printf("bcm2835_i2c_write_read_rs failed at %s:%d, errCode = 0x%x\n",__FILE__,__LINE__,errCode);
		return;
	}
	D1 = 0;
	D1 = recvBuf[0];
	D1 = (D1 << 8) | recvBuf[1];
	D1 = (D1 << 8) | recvBuf[2];

	// Request D2 conversion
	//sendBuf[0] = MS5837_CONVERT_D2_8192;
	sendBuf[0] = MS5837_CONVERT_D2_2048;
	if((errCode = bcm2835_i2c_write(sendBuf,1)))
	{
		printf("bcm2835_i2c_write failed at %s:%d, errCode = 0x%x\n",__FILE__,__LINE__,errCode);
		return;
	}
	//bcm_2835_delay(20); // Max conversion time per datasheet
	bcm2835_delay(5);  // When OSR = 2048, wait MS5837 converts, need 5ms

	sendBuf[0] = MS5837_ADC_READ;
	if((errCode = bcm2835_i2c_write_read_rs(sendBuf,1,recvBuf,3)))
	{
		printf("bcm2835_i2c_write_read_rs failed at %s:%d, errCode = 0x%x\n",__FILE__,__LINE__,errCode);
		return;
	}
	D2 = 0;
	D2 = recvBuf[0];
	D2 = (D2 << 8) | recvBuf[1];
	D2 = (D2 << 8) | recvBuf[2];

	depth_calculate();
}

float pressure(float conversion) {
	return P*conversion;
}

float temperature() {
	return TEMP/100.0f;
}

float depth() {
	return (pressure(mbar)-101300)/(fluidDensity*9.80665);
}

float altitude() {
	return (1-pow((pressure()/1013.25),.190284))*145366.45*.3048;
}

void autoCalibration()
{
	printf("Auto Calibration...");
	depth_read();
	float rawDepth = depth();
	offsetP = -rawDepth*fluidDensity*9.80665/100;
	depth_read();
	float newDepth = depth();
	printf("Current depth = %f, calculate offsetP = %d, corrected depth = %f",rawDepth,offsetP,newDepth);
}

bool depth_init() {

	//Init bcm2835_i2c;
	printf("Init...\n");
	if(!bcm2835_i2c_begin())
	{
		printf("bcm2835 i2c begin failed at %s:%d\n",__FILE__,__LINE__);
		exit(-1);
	}
	bcm2835_i2c_setSlaveAddress(MS5837_ADDR);
	bcm2835_i2c_set_baudrate(100000);

	sendBuf[0] = MS5837_RESET;
	// Reset the MS5837, per datasheet
	if((errCode = bcm2835_i2c_write(sendBuf,1)))
		printf("bcm2835_i2c_write failed at %s:%d, errCode = 0x%x\n",__FILE__,__LINE__,errCode);

	// Wait for reset to complete
	bcm2835_delay(10);

	// Read calibration values and CRC
	for ( uint8_t i = 0 ; i < 7 ; i++ ) {
		sendBuf[0] = MS5837_PROM_READ+i*2;
		if((errCode =bcm2835_i2c_write_read_rs(sendBuf,1,recvBuf,2)))
			printf("bcm2835_i2c_write_read_rs failed at %s:%d, errCode = 0x%x\n",__FILE__,__LINE__,errCode);
		C[i] = (recvBuf[0]<<8) | recvBuf[1];
	}

	// Verify that data is correct with CRC
	uint8_t crcRead = C[0] >> 12;
	uint8_t crcCalculated = crc4(C);

	if ( crcCalculated == crcRead ) {
		autoCalibration();
		return true; // Initialization success
	}

	return false; // CRC fail
}
