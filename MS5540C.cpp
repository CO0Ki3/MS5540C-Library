#include "MS5540C.h"

float water_type;

const int clock = 6;
const int get_interval = 35;
static int ms5540c_state = 0;

float g;
float latitude = 37.0;
float lat_rad = ((37.0/57.29578) * PI / 180);
float x = sin(lat_rad)*sin(lat_rad);

long pressure;
float temperature;
float waterdepth;


void ms5540c_reset() {
  SPI.setDataMode(SPI_MODE0);
  SPI.transfer(0x15);
  SPI.transfer(0x55);
  SPI.transfer(0x40);
}

void ms5540c_setup() {
  SPI.begin();
  SPI.setBitOrder(MSBFIRST);
  SPI.setClockDivider(SPI_CLOCK_DIV32);
  pinMode(clock, OUTPUT);
  delay(100);
}

void ms5540c_loop(float water_type) {
  static uint32_t tTime;

  unsigned int word1 = 0;
  unsigned int word11 = 0;
  unsigned int word2 = 0;
  unsigned int word3 = 0;
  byte word22 = 0;
  byte word33 = 0;
  unsigned int word4 = 0;
  byte word44 = 0;
  static long c1;
  static long c2;
  static long c3;
  static long c4;
  static long c5;
  static long c6;
  unsigned int presMSB = 0;
  unsigned int presLSB = 0;
  static unsigned int D1 = 0;
  unsigned int tempMSB = 0;
  unsigned int tempLSB = 0;
  static unsigned int D2 = 0;
  long UT1 = 0;
  long dT = 0;
  long TEMP = 0;
  long OFF = 0;
  long SENS = 0;

  float p;
  static long PCOMP = 0;
  static long PCOMP2 = 0;
  static long PH2 = 0;
  static float TEMPREAL = 0;
  static float DEPTH = 0;

  long dT2 = 0;
  static float TEMPCOMP = 0;

  bool ret = false;

  switch( ms5540c_state )
  {
    case 0:
      //TCCR4B = (TCCR4B & 0xF8) | 1 ;
      TCCR1B = (TCCR1B & 0b11111000) | 0x01;
      analogWrite (clock, 128) ;
      ms5540c_reset();

      word1 = 0;
      word11 = 0;
      SPI.transfer(0x1D);
      SPI.transfer(0x50);
      SPI.setDataMode(SPI_MODE1);
      word1 = SPI.transfer(0x00);
      word1 = word1 << 8;
      word11 = SPI.transfer(0x00);
      word1 = word1 | word11;
      ms5540c_reset();

      word2 = 0;
      word22 = 0;
      SPI.transfer(0x1D);
      SPI.transfer(0x60);
      SPI.setDataMode(SPI_MODE1);
      word2 = SPI.transfer(0x00);
      word2 = word2 <<8;
      word22 = SPI.transfer(0x00);
      word2 = word2 | word22;
      ms5540c_reset();

      word3 = 0;
      word33 = 0;
      SPI.transfer(0x1D);
      SPI.transfer(0x90);
      SPI.setDataMode(SPI_MODE1);
      word3 = SPI.transfer(0x00);
      word3 = word3 <<8;
      word33 = SPI.transfer(0x00);
      word3 = word3 | word33;
      ms5540c_reset();

      word4 = 0;
      word44 = 0;
      SPI.transfer(0x1D);
      SPI.transfer(0xA0);
      SPI.setDataMode(SPI_MODE1);
      word4 = SPI.transfer(0x00);
      word4 = word4 <<8;
      word44 = SPI.transfer(0x00);
      word4 = word4 | word44;

      c1 = (word1 >> 1);
      c2 = ((word3 & 0x3F) << 6) | ((word4 & 0x3F));
      c3 = (word4 >> 6);
      c4 = (word3 >> 6);
      c5 = (word2 >> 6) | ((word1 & 0x1) << 10);
      c6 = (word2 & 0x3F);
      ms5540c_reset();

      SPI.transfer(0x0F);
      SPI.transfer(0x20);

      tTime = millis();
      ms5540c_state = 1;
      break;

    case 1:
      if( (millis()-tTime) >= get_interval ) {
        ms5540c_state = 2;
      }
      break;

    case 2:
      SPI.setDataMode(SPI_MODE1);
      tempMSB = SPI.transfer(0x00);
      tempMSB = tempMSB << 8;
      tempLSB = SPI.transfer(0x00);
      D2 = tempMSB | tempLSB;
      ms5540c_reset();

      SPI.transfer(0x0F);
      SPI.transfer(0x40);

      tTime = millis();
      ms5540c_state = 3;
      break;

    case 3:
      if( (millis()-tTime) >= get_interval ) {
        ms5540c_state = 4;
      }
      break;

    case 4:
      SPI.setDataMode(SPI_MODE1);
      presMSB = SPI.transfer(0x00);
      presMSB = presMSB << 8;
      presLSB = SPI.transfer(0x00);
      D1 = presMSB | presLSB;
      UT1 = (c5 * 8) + 20224;
      dT =(D2 - UT1);
      TEMP = 200 + ((dT * (c6 + 50))/1024);
      OFF = (c2*4) + (((c4 - 512) * dT)/4096);
      SENS = c1 + ((c3 * dT)/1024) + 24576;

      PCOMP = ((((((SENS * (D1 - 7168))/16384)- OFF)*10)/32)+(250*10))/10;

      if(PCOMP > 1000) {
        PH2 = (-5*((PCOMP-1000)*(PCOMP-1000)));
        PH2 <<= 19;
       //PH2 = (-5*((PCOMP-1000)*(PCOMP-1000)))/(1<<19);
       //unsigned long toShift = 1UL << 16; 꼴로 변형
      } else {
        PH2 = 0;
      }
      PCOMP2 = PCOMP - PH2;

      if(PCOMP2 < 1013.25) PCOMP2 = 0;

      TEMPREAL = TEMP/10;

      dT2 = dT - ((dT >> 7 * dT >> 7) >> 3);
      TEMPCOMP = (200 + (dT2*(c6+100) >>11))/10;

      if(water_type == FRESH) {
        DEPTH = PCOMP2 * 1.019716;
      } else {
        g = 9.780318*(1.0 + ((5.2788*1/1000) + 2.36*1/100000 * x)*x) + (1.092*1/1000000*PCOMP2);
        DEPTH = (((((-1.82*1/1000000000000000)*PCOMP2 + (2.279*1/10000000000))*PCOMP2 - (2.2512*1/100000))*PCOMP2 + 9.72659)*PCOMP2)/g;
      }

      pressure = PCOMP;
      temperature = TEMPCOMP;
      waterdepth = DEPTH/10000;

      ret = true;
      ms5540c_state = 0;
      break;

    default:
      ms5540c_state = 0;
      break;
  }
}
