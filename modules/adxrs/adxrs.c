/**
 * @cond internal
 * @file
 */
#include <stdint.h>
#include <math.h>
#include <avr/cpufunc.h>
#include <util/atomic.h>
#include <util/parity.h>
#include <clock/defs.h>
#include <util/delay.h>
#include "adxrs.h"
#include "adxrs_config.h"

#define T int16_t
#include "fifo.h"
#undef T

#if (CLOCK_PER_FREQ / ADXRS_SPI_PRESCALER) > 8080000
# error ADXRS_SPI_PRESCALER is too low, max ADXRS SPI frequency is 8.08MHz
#endif

// Get SPI to use and the interrupt vector

// Command to duplicate SPIC code for each SPI.
// Vimmers will use it with " :*! ".
//   python -c 'import sys; s=sys.stdin.read(); print "\n".join(s.replace("C",x) for x in "CDEF")'

#ifdef ADXRS_SPIC_ENABLE
# ifdef ADXRS_SPI
#  error Multiple ADXRS_SPIx_ENABLE defined can be defined
# endif
# define ADXRS_SPI  SPIC
# define ADXRS_SPI_INT_vect  SPIC_INT_vect
#endif

#ifdef ADXRS_SPID_ENABLE
# ifdef ADXRS_SPI
#  error Multiple ADXRS_SPIx_ENABLE defined can be defined
# endif
# define ADXRS_SPI  SPID
# define ADXRS_SPI_INT_vect  SPID_INT_vect
#endif

#ifdef ADXRS_SPIE_ENABLE
# ifdef ADXRS_SPI
#  error Multiple ADXRS_SPIx_ENABLE defined can be defined
# endif
# define ADXRS_SPI  SPIE
# define ADXRS_SPI_INT_vect  SPIE_INT_vect
#endif

#ifdef ADXRS_SPIF_ENABLE
# ifdef ADXRS_SPI
#  error Multiple ADXRS_SPIx_ENABLE defined can be defined
# endif
# define ADXRS_SPI  SPIF
# define ADXRS_SPI_INT_vect  SPIF_INT_vect
#endif

#ifndef ADXRS_SPI_INT_vect
# error No defined ADXRS_SPIx_ENABLE
#endif


#define CALIBRATION_SAMPLES_LENGTH 101

/** @brief ADXRS gyro data
 *
 * The \e response field contains data of the last received response, i.e. the
 * response to the penultimate command.
 * It is updated by \ref adxrs_parse_response().
 */
typedef struct {
  portpin_t cspp;  ///< port pin of the inversed CS pin
  adxrs_response_t response;  ///< response to the penultimate command
  float angle;  ///< current angle for capture mode
  float capture_scale;  ///< scaling coefficient for captured values
  uint8_t capture_index;  ///< index of next captured byte
  int16_t capture_speed;  ///< last (valid) captured angle speed

  struct {
    bool mode;  ///< calibration mode
    int16_t offset;  ///< calibration offset
    float offset_sqsd; ///< calibration squared sd

    float sum;
    float sqsum;
    fifo_t samples;
    int16_t samples_buffer[CALIBRATION_SAMPLES_LENGTH];
  } calibration;
} adxrs_t;


/// The gyro
static adxrs_t gyro;


void adxrs_init(portpin_t cspp)
{
  gyro.cspp = cspp;
  gyro.response.type = ADXRS_RESPONSE_NONE;
  gyro.angle = 0;
  gyro.calibration.mode = false;
  gyro.calibration.offset = 0;

  fifo_init(&gyro.calibration.samples,
    gyro.calibration.samples_buffer,
    CALIBRATION_SAMPLES_LENGTH);

  // initialize SPI
  portpin_dirset(&PORTPIN_SPI_SS(&ADXRS_SPI));
  ADXRS_SPI.CTRL = SPI_ENABLE_bm | SPI_MASTER_bm | SPI_MODE_0_gc |
#if ADXRS_SPI_PRESCALER == 2
      SPI_PRESCALER_DIV4_gc | SPI_CLK2X_bm
#elif ADXRS_SPI_PRESCALER == 4
      SPI_PRESCALER_DIV4_gc
#elif ADXRS_SPI_PRESCALER == 8
      SPI_PRESCALER_DIV16_gc | SPI_CLK2X_bm
#elif ADXRS_SPI_PRESCALER == 16
      SPI_PRESCALER_DIV16_gc
#elif ADXRS_SPI_PRESCALER == 32
      SPI_PRESCALER_DIV64_gc | SPI_CLK2X_bm
#elif ADXRS_SPI_PRESCALER == 64
      SPI_PRESCALER_DIV64_gc
#elif ADXRS_SPI_PRESCALER == 128
      SPI_PRESCALER_DIV128_gc
#else
# error Invalid ADXRS_SPI_PRESCALER value
#endif
      ;

  portpin_dirset(&PORTPIN_SPI_MOSI(&ADXRS_SPI));
  portpin_dirclr(&PORTPIN_SPI_MISO(&ADXRS_SPI));
  portpin_dirset(&PORTPIN_SPI_SCK(&ADXRS_SPI));

  portpin_dirset(&cspp);
  portpin_outset(&cspp);
}


/// Check command response parity
static bool adxrs_check_response_parity(uint8_t data[4])
{
  uint8_t parity;
  parity = data[0] ^ data[1];
  if(parity_even_bit(parity) == 0) {
    return false;
  }
  parity ^= data[2] ^ data[3];
  if(parity_even_bit(parity) == 0) {
    return false;
  }
  return true;
}

/// Parse command response
static void adxrs_parse_response(uint8_t data[4])
{
  // check parity
  if(!adxrs_check_response_parity(data)) {
    gyro.response.type = ADXRS_RESPONSE_BAD_PARITY;
    return;
  }

  // parse response fields
  uint8_t status = (data[0] >> 2) & 0x3;
  if(status == 3) {
    uint8_t type = (data[0] >> 5) & 0x7;
    if(type == 0) {
      // R/W error
      gyro.response.type = ADXRS_RESPONSE_RW_ERROR;
      gyro.response.rw_error.spi_error = (data[1] >> 2) & 1;
      gyro.response.rw_error.request_error = (data[1] >> 1) & 1;
      gyro.response.rw_error.data_unavailable = data[1] & 1;
    } else {
      // read/write
      if(type == 1) {
        gyro.response.type = ADXRS_RESPONSE_WRITE;
      } else if(type == 2) {
        gyro.response.type = ADXRS_RESPONSE_READ;
      } else {
        // unknown type
        gyro.response.type = ADXRS_RESPONSE_INVALID;
      }
      gyro.response.read.data = ((uint16_t)(data[1] & 0x1f) << 11) |
          ((uint16_t)data[2] << 3) | (data[3] >> 5);
    }
  } else {
    // sensor data
    gyro.response.type = ADXRS_RESPONSE_SENSOR_DATA;
    gyro.response.sensor_data.sequence = (data[0] >> 5) & 0x7;
    gyro.response.sensor_data.status = status;
    gyro.response.sensor_data.data = ((uint16_t)(data[0] & 0x03) << 14) |
        ((uint16_t)data[1] << 6) | (data[2] >> 2);
    gyro.response.sensor_data.fault_raw = data[3] & 0xfe;
  }
}


/// Send and receive a single byte to/from SPI
static uint8_t adxrs_spi_transmit(uint8_t data)
{
  ADXRS_SPI.DATA = data;
  while(!(ADXRS_SPI.STATUS & SPI_IF_bm)) ;
  return ADXRS_SPI.DATA;
}


void adxrs_cmd_raw(uint8_t data[4])
{
  // send/receive data
  portpin_outclr(&gyro.cspp);
  uint8_t rdata[4];
  rdata[0] = adxrs_spi_transmit(data[0]);
  rdata[1] = adxrs_spi_transmit(data[1]);
  rdata[2] = adxrs_spi_transmit(data[2]);
  rdata[3] = adxrs_spi_transmit(data[3]);
  portpin_outset(&gyro.cspp);
  adxrs_parse_response(rdata);
}

void adxrs_cmd_sensor_data(uint8_t seq, bool chk)
{
  uint8_t p = (seq >> 1) ^ (seq >> 1) ^ seq ^ chk;
  uint8_t data[4] = { ((seq & 3) << 6) | 0x20 | ((seq & 7) << 2),
    0, 0, p | (chk << 1) };
  adxrs_cmd_raw(data);
}

void adxrs_cmd_read(uint8_t addr)
{
  uint8_t p = parity_even_bit(addr);
  uint8_t data[4] = { 0x80 | ((addr >> 7) & 0x1), addr << 1, 0, p };
  adxrs_cmd_raw(data);
}

void adxrs_cmd_write(uint8_t addr, uint16_t value)
{
  uint8_t p = parity_even_bit(addr ^ (uint8_t)value ^ (uint8_t)(value >> 8));
  uint8_t data[4] = { 0x40 | addr >> 7, (addr << 1) | ((value >> 15) & 0x1),
    value >> 7, ((uint8_t)value << 1) | p };
  adxrs_cmd_raw(data);
}

adxrs_response_t const *adxrs_get_response(void)
{
  return &gyro.response;
}


bool adxrs_startup(void)
{
  _delay_ms(100);
  adxrs_cmd_sensor_data(0, 1);
  _delay_ms(50);
  adxrs_cmd_sensor_data(0, 0);
  _delay_ms(50);
  adxrs_cmd_sensor_data(0, 0);
  if(gyro.response.type != ADXRS_RESPONSE_SENSOR_DATA
     || gyro.response.sensor_data.status != 2
     || gyro.response.sensor_data.fault_raw != 0xfe) {
    return false;
  }

  _delay_ms(50);
  adxrs_cmd_sensor_data(0, 0);
  if(gyro.response.type != ADXRS_RESPONSE_SENSOR_DATA
     || gyro.response.sensor_data.status != 2
     || gyro.response.sensor_data.fault_raw != 0xfe) {
    return false;
  }

  _delay_ms(50);
  adxrs_cmd_sensor_data(0, 0);
  if(gyro.response.type != ADXRS_RESPONSE_SENSOR_DATA
     || gyro.response.sensor_data.status != 1
     || gyro.response.sensor_data.fault_raw != 0) {
    return false;
  }

  return true;
}


void adxrs_capture_start(float scale)
{
  gyro.angle = 0;
  gyro.capture_scale = scale;
  gyro.capture_index = 0;
  gyro.capture_speed = 0;

  // Send a sensor data command to be sure that the first response received by
  // the interrupt handler is a sensor data response with an up-to-date value.
  portpin_outclr(&gyro.cspp);
  adxrs_spi_transmit(0x20);
  adxrs_spi_transmit(0x00);
  adxrs_spi_transmit(0x00);
  adxrs_spi_transmit(0x00);
  portpin_outset(&gyro.cspp);

  // Enable interruptions and start the capture with the first command byte
  ADXRS_SPI.INTCTRL = ADXRS_CAPTURE_INTLVL;
  // /CS must be high for 100ns, or 3.2 cycles at 32MHz (max frequency)
  // add some nops juste to be sure
  _NOP(); _NOP(); _NOP();
  portpin_outclr(&gyro.cspp);
  ADXRS_SPI.DATA = 0x20;
}

void adxrs_capture_stop(void)
{
  ADXRS_SPI.INTCTRL = 0;
  gyro.response.type = ADXRS_RESPONSE_NONE;
  portpin_outset(&gyro.cspp);
}

void adxrs_calibration_mode(bool activate)
{
  gyro.calibration.mode = activate;
}

bool adxrs_get_calibration_mode()
{
  return gyro.calibration.mode;
}

float adxrs_get_angle(void)
{
  float angle;
  INTLVL_DISABLE_ALL_BLOCK() {
    angle = gyro.angle;
  }
  return angle;
}

void adxrs_set_angle(float angle)
{
  INTLVL_DISABLE_ALL_BLOCK() {
    gyro.angle = angle;
  }
}

float adxrs_get_speed(void) {
  float speed;
  INTLVL_DISABLE_ALL_BLOCK() {
    speed = gyro.capture_speed;
  }
  return speed;
}

int16_t adxrs_get_offset() {
  int16_t offset;
  INTLVL_DISABLE_ALL_BLOCK() {
    offset = gyro.calibration.offset;
  }
  return offset;
}

float adxrs_get_offset_sqsd() {
  float sqsd;
  INTLVL_DISABLE_ALL_BLOCK() {
    sqsd = gyro.calibration.offset_sqsd;
  }
  return sqsd;
}

// Update captured angle value
static void adxrs_update_angle(uint8_t data[4])
{
  if(adxrs_check_response_parity(data) && (data[0] & 0x0C) == 0x04) {
    // valid response, parse speed
    gyro.capture_speed = ((uint16_t)(data[0] & 0x03) << 14) |
        ((uint16_t)data[1] << 6) | (data[2] >> 2);
  }
  else
    return;

  // check calibration signal rising edge
  static bool lcalibration = false;
  if(gyro.calibration.mode && !lcalibration) {
    gyro.calibration.offset = gyro.capture_speed;
  }
  lcalibration = gyro.calibration.mode;

  if(gyro.calibration.mode) {
    //gyro.calibration.offset = 0.95*gyro.calibration_offset + 0.05*gyro.capture_speed;

    // update sum
    float v = gyro.capture_speed;
    if(fifo_isfull(&gyro.calibration.samples)) {
      float ov = fifo_pop(&gyro.calibration.samples);
      gyro.calibration.sum -= ov;
      gyro.calibration.sqsum -= ov*ov;
    }
    gyro.calibration.sum += v;
    gyro.calibration.sqsum += v*v;
    fifo_push(&gyro.calibration.samples, v);

    // compute mean value and mean of squared values
    int n = fifo_size(&gyro.calibration.samples);
    float mean = gyro.calibration.sum/n;
    float sqmean = gyro.calibration.sqsum/n;

    // compute squared standard dev of measure
    float sqsd = 1.0*(n*sqmean - mean*mean)/(n*n);

    // update gyro offset
    gyro.calibration.offset = mean;
    gyro.calibration.offset_sqsd = sqsd;
  }
  else {
    // update angle (internal) value
    // on error, previous (valid) speed value is used
    gyro.capture_speed = gyro.capture_speed - gyro.calibration.offset;
    float angle = gyro.angle + gyro.capture_speed * gyro.capture_scale;
    INTLVL_DISABLE_ALL_BLOCK() {
      gyro.angle = angle;
    }
  }
}


void adxrs_capture_manual(float scale)
{
  // send next capture command
  uint8_t rdata[4];
  portpin_outclr(&gyro.cspp);
  rdata[0] = adxrs_spi_transmit(0x20);
  rdata[1] = adxrs_spi_transmit(0x00);
  rdata[2] = adxrs_spi_transmit(0x00);
  rdata[3] = adxrs_spi_transmit(0x00);
  portpin_outset(&gyro.cspp);

  if(scale != 0) {
    // check response, update angle
    gyro.capture_scale = scale;
    adxrs_update_angle(rdata);
  } else {
    // reset current speed
    gyro.capture_speed = 0;
  }
}


/// Interrupt handler for capture mode
ISR(ADXRS_SPI_INT_vect)
{
  static uint8_t data[4];  // current capture data

  // command sent is always the same: 0x20000000
  // only the first byte has a not-null value, this is convenient
  data[gyro.capture_index] = ADXRS_SPI.DATA;
  if(++gyro.capture_index == 4) {
    portpin_outset(&gyro.cspp);

    // check response, update angle
    adxrs_update_angle(data);

    // first byte of a new command
    portpin_outclr(&gyro.cspp);
    gyro.capture_index = 0;
    ADXRS_SPI.DATA = 0x20;
  } else {
    // the command continue
    ADXRS_SPI.DATA = 0x00;
  }
}


///@endcond
