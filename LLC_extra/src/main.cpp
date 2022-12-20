#include <avr/io.h>
#include <avr/interrupt.h>
#include <util/atomic.h>
#include <util/delay.h>

#define DATA_OUT 1
#define DATA_IN 0
#define CLK 2
#define CS 4

unsigned short T1, P1;
signed short T2, T3, P2, P3, P4, P5, P6, P7, P8, P9;
long signed int adc_T, adc_P, t_fine;
uint8_t tiental, eenheid, tiende, honderdste;
uint8_t honderdFahr, tientalFahr, eenheidFahr, tiendeFahr, honderdsteFahr;
uint8_t pHonderd, pTien, pEen;

void init()
{
  cli();
  // set USICR to zero
  USICR = 0;
  // set three-wire mode
  USICR |= (1 << USIWM0);
  // set external clock positive edge
  USICR |= (1 << USICS1) | (1 << USICLK);
  // set DDRB to zero
  DDRB = 0;
  // set output pins
  DDRB |= (1 << DD2) | (1 << DATA_OUT) | (1 << DD3) | (1 << CS);
  // set PORTB to zero
  PORTB = 0;
  PINB = 0;
  // set CS high
  PINB |= (1 << PIN4);
  PINB |= (1 << PIN3);

  // set prescaler to 16384
  TCCR1 |= (1 << CS13) | (1 << CS12) | (1 << CS11) | (1 << CS10);
  // CTC mode
  TCCR1 |= (1 << CTC1);

  OCR1A = F_CPU / 16384.0 / 2 - 1;
  OCR1C = OCR1A;

  // activate interrupt
  TIMSK |= (1 << OCIE1A);

  // set USICLK high
  USICR |= (1 << USICLK);

  sei();
}

// function that transfers and receives data.
uint8_t transfer(uint8_t data)
{
  // reset 4-bit counter and clear overflow flag.
  USISR |= (1 << USIOIF);
  // store data to be sent in USIDR.
  USIDR = data; 
  // while USIOIF not high keep toggling clock.
  ATOMIC_BLOCK(ATOMIC_RESTORESTATE)
  {
    while (! (USISR & (1 << USIOIF))) {
      USICR |= (1 << USITC);
    }
  }
  return USIDR;
}

// function to write digit to 7-segment
void writeDigit(int i)
{
  // data to display numbers stored in number[] array
  uint8_t number[10] = {0b00111111, 0b0000110, 0b01011011, 0b01001111, 0b01100110, 0b01101101, 0b01111101, 0b00000111, 0b01111111, 0b01100111};
  // set latch pin low for SPI
  PINB |= (1 << PIN3);
  transfer(number[i]);
  // set latch pin high after transfer
  PINB |= (1 << PIN3);
}

void sensorTest()
{
  // set CSB low
  PINB |= (1 << CS);
  // send 0xD0 over SPI
  transfer(0xD0);
  // set CSB high
  PINB |= (1 << CS);
}

void storeCalibrationData()
{
  PINB |= (1 << CS);
  transfer(0x88);
  // storing T1 in int variable
  uint8_t lowByteT1 = transfer(0);
  uint8_t highByteT1 = transfer(0);
  // storing T2 in int variable
  uint8_t lowByteT2 = transfer(0);
  uint8_t highByteT2 = transfer(0);
  // storing T3 in int variable
  uint8_t lowByteT3 = transfer(0);
  uint8_t highByteT3 = transfer(0);

  uint8_t lowP1 = transfer(0);
  uint8_t highP1 = transfer(0);

  uint8_t lowP2 = transfer(0);
  uint8_t highP2 = transfer(0);

  uint8_t lowP3 = transfer(0);
  uint8_t highP3 = transfer(0);
  
  uint8_t lowP4 = transfer(0);
  uint8_t highP4 = transfer(0);

  uint8_t lowP5 = transfer(0);
  uint8_t highP5 = transfer(0);

  uint8_t lowP6 = transfer(0);
  uint8_t highP6 = transfer(0);

  uint8_t lowP7 = transfer(0);
  uint8_t highP7 = transfer(0);

  uint8_t lowP8 = transfer(0);
  uint8_t highP8 = transfer(0);

  uint8_t lowP9 = transfer(0);
  uint8_t highP9 = transfer(0);
  PINB |= (1 << CS);

  T1 = (highByteT1 << 8) | lowByteT1;
  T2 = (highByteT2 << 8) | lowByteT2;
  T3 = (highByteT3 << 8) | lowByteT3;

  P1 = (highP1 << 8) | lowP1;
  P2 = (highP2 << 8) | lowP2;
  P3 = (highP3 << 8) | lowP3;
  P4 = (highP4 << 8) | lowP4;
  P5 = (highP5 << 8) | lowP5;
  P6 = (highP6 << 8) | lowP6;
  P7 = (highP7 << 8) | lowP7;
  P8 = (highP8 << 8) | lowP8;
  P9 = (highP9 << 8) | lowP9;
}

void myDelay()
{
  for (long i = 0; i < F_CPU / 14; i++)
  {
    asm("nop");
  }
}

void configureSensorMode()
{
  PINB |= (1 << CS);
  transfer(0x74);
  transfer(0b01001011);
  PINB |= (1 << CS);
}

float tempData()
{
  long signed int result, result2;
  long signed int var1, var2;
  float temp;

  PINB |= (1 << CS);
  transfer(0xFA);
  // reading and storing temp data in variables
  uint8_t highByteTemp = transfer(0);
  uint8_t lowByteTemp = transfer(0);
  uint8_t extLowByteTemp = transfer(0);
  PINB |= (1 << CS);
  // storing tempData in adc_T
  result = (highByteTemp << 8) | lowByteTemp;
  result2 = (result << 8) | extLowByteTemp;
  adc_T = (result2 >> 4);
  // calculation from datasheet 3.12
  var1 = (((double)adc_T) / 16384.0 - ((double)T1) / 1024.0) * ((double)T2);
  var2 = ((((double)adc_T) / 131072.0 - ((double)T1) / 8192.0) * (((double)adc_T) / 131072.0 - ((double)T1) / 8192.0)) * ((double)T3);
  t_fine = var1 + var2;
  temp = (var1 + var2) / 5120.0;
  
  tiental = (unsigned int) (temp / 10);
  eenheid = (unsigned int) temp % 10;
  tiende = ((uint8_t)(temp * 10)) % 10;
  honderdste = ((uint8_t)(temp * 100)) % 10;

  return temp;
}

void pressureData()
{
  long signed int result, result2;
  long signed int var1, var2;
  float pressure;
  uint32_t p2;

  PINB |= (1 << CS);
  transfer(0xF7);

  uint8_t highPressure = transfer(0);
  uint8_t lowPressure = transfer(0);
  uint8_t extPressure = transfer(0);
  PINB |= (1 << CS);

  result = (highPressure << 8) | lowPressure;
  result2 = (result << 8) | extPressure;
  adc_P = (result2 >> 4);
  // pressure conversion calculation from datasheet 3.12
  var1 = ((double)t_fine / 2.0) - 64000.0;
  var2 = var1 * var1 * ((double)P6) / 32768.0;
  var2 = var2 + var1 * ((double)P5) * 2.0;
  var2 = (var2 / 4.0) + (((double)P4) * 65536.0);
  var1 = (((double)P3) * var1 * var1 / 524288.0 + ((double)P2) * var1) / 524288.0;
  var1 = (1.0 + var1 / 32768.0) * ((double)P1);
  pressure = 1048576.0 - (double)adc_P;
  pressure = (pressure - (var2 / 4096.0)) * 6250.0 / var1;
  var1 = ((double)P9) * pressure * pressure / 2147483648.0;
  var2 = pressure * ((double)P8) / 32768.0;
  pressure = pressure + (var1 + var2 + ((double)P7)) / 16.0;
  // convert pressure to int
  p2 = pressure;
  // convert pressure value to single digits 
  pHonderd = p2 / 100000;
  pTien = (p2 / 10000) % 10;
  pEen = (p2 / 1000) % 10;
}

float toFahrenheit()
{
  float temp = tempData();
  float fahrenheit;

  fahrenheit = temp * 1.8 + 32;


  honderdFahr = (unsigned int) fahrenheit / 100;
  tientalFahr = (unsigned int) (fahrenheit / 10) % 10;
  eenheidFahr = (unsigned int) fahrenheit % 10;
  tiendeFahr = (unsigned int) (fahrenheit * 10) % 10;
  honderdsteFahr = (unsigned int) (fahrenheit * 100) % 10;

  return fahrenheit;
}

int main()
{
  init();
  configureSensorMode();
  storeCalibrationData();

  while (true)
  {
    writeDigit(tientalFahr);
    _delay_ms(500);
    writeDigit(eenheidFahr);
    _delay_ms(500);
    writeDigit(tiendeFahr);
    _delay_ms(500);
  }

  return 0;
}

ISR(TIM1_COMPA_vect)
{
  tempData();
  pressureData();
  toFahrenheit();
}

