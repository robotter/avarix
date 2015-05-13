/** @defgroup adxrs ADXRS gyro
 * @brief Analog Devices' ADXRS453 gyro
 *
 * This module handles a single gyro.
 * There are two operation modes: polling mode and capture mode. One can switch
 * from one mode to the other but only methods of the currently active mode can
 * be used.
 *
 * In polling mode, commands are sent manually through \e adxrs_cmd_*() methods.
 * When a command in sent, the penultimate response is parsed and can be
 * retrieved with \ref adxrs_get_response().
 *
 * In capture mode, sensor data commands are sent repeatedly using SPI
 * interrupt. Angle speed is retrieved and integrated; the angle position
 * value can be retrieved with \ref adxrs_get_angle().
 * A manual capture mode is available; capture are not interrupt-based but
 * triggered manually.
 *
 * SM bits (sensor module) are not handled.
 * They are hard-coded to 000 on ADXRS453.
 *
 * Read and write commands use 9-bit addresses. But since the device memory map
 * only define low addresses, the 9th bit is not needed and the module uses
 * 8-bit addresses.
 */
//@{
/**
 * @file
 * @brief Definitions for ADXRS gyro
 */
#ifndef ADXRS_H
#define ADXRS_H

#include <avr/io.h>
#include <stdint.h>
#include <stdbool.h>
#include <avarix/portpin.h>


/// ADXRS response type
typedef enum {
  ADXRS_RESPONSE_NONE = 0,  ///< no response (initial state)
  ADXRS_RESPONSE_SENSOR_DATA,  ///< sensor data command response
  ADXRS_RESPONSE_READ,  ///< read command response
  ADXRS_RESPONSE_WRITE,  ///< write command response
  ADXRS_RESPONSE_RW_ERROR,  ///< R/W error response
  ADXRS_RESPONSE_BAD_PARITY,  ///< bad parity in response
  ADXRS_RESPONSE_INVALID,  ///< invalid response format
} adxrs_response_type_t;


/// Fault register (low byte)
typedef struct {
#if __LITTLE_ENDIAN || DOXYGEN
  uint8_t :1;
  uint8_t chk:1;  ///< check bit, to generate faults
  uint8_t cst:1;  ///< continuous self-test failure or amplitude detection failed
  uint8_t pwr:1;  ///< power regulation failed
  uint8_t por:1;  ///< power-on or reset failed to initialize
  uint8_t nvm:1;  ///< nonvolatile memory fault
  uint8_t q:1;  ///< quadrature error
  uint8_t pll:1;  ///< phase-locked loop failure
#else
  uint8_t pll:1;
  uint8_t q:1;
  uint8_t nvm:1;
  uint8_t por:1;
  uint8_t pwr:1;
  uint8_t cst:1;
  uint8_t chk:1;
  uint8_t :1;
#endif

} adxrs_fault0_t;


/// ADXRS response data
typedef union {

  adxrs_response_type_t type;

  struct {
    adxrs_response_type_t type;
    uint8_t sequence:3;
    uint8_t status:2;
    uint16_t data;
    union {
      uint8_t fault_raw;
      adxrs_fault0_t fault;
    };
  } sensor_data;

  struct {
    adxrs_response_type_t type;
    uint16_t data;
  } read;

  struct {
    adxrs_response_type_t type;
    uint16_t data;
  } write;

  struct {
    adxrs_response_type_t type;
    uint8_t spi_error:1;
    uint8_t request_error:1;
    uint8_t data_unavailable:1;
    adxrs_fault0_t fault;
  } rw_error;

} adxrs_response_t;



/** @brief Initialize the gyro
 *
 * @param cspp  port pin of the CS pin
 */
void adxrs_init(portpin_t cspp);


/** @name Polling mode methods
 *
 * These method must not be used whil capture mode is active.
 */
//@{

/** @brief Send a raw command
 *
 * @param data  raw command data, bytes are in transmission order
 *
 * @note Parity bit is not modified.
 */
void adxrs_cmd_raw(uint8_t data[4]);

/** @brief Send a sensor data command
 *
 * @param seq  sequence bits (SQ)
 * @param chk  CHK bit
 */
void adxrs_cmd_sensor_data(uint8_t seq, bool chk);

/** @brief Send a read command
 *
 * @param addr  register address to read
 */
void adxrs_cmd_read(uint8_t addr);

/** @brief Send a write command
 *
 * @param addr  register address to write
 * @param value  new register value
 */
void adxrs_cmd_write(uint8_t addr, uint16_t value);

/** @brief Return the reponse to the penultimate command
 * @note Response is not updated while in capture mode.
 */
adxrs_response_t const *adxrs_get_response(void);

/** @brief Send a start-up sequence
 *
 * Execute the recommended start-up sequence with CHK bit assertion.
 * Return true on success, false on error (unexpected reply).
 */
bool adxrs_startup(void);

//@}


/** @name Capture mode methods
 *
 * In capture mode, gyro angle speed is continuously polled and integrated
 * using SPI interruption.
 * Current angle value can be retrieved at any moment.
 * Capture period is determined by SPI period.
 *
 * An alternate mode not using interruptions is available. Values are retrieved
 * manually using \ref adxrs_capture_manual().
 *
 * Angle values are in radians, between -Pi and Pi.
 */
//@{

/** @brief Start capture mode
 *
 * \a scale is a coefficient used to integrate angle speeds that should be
 * calibrated.
 * Internally, it includes the capture period and the coefficient to convert
 * gyro values to milliradians.
 *
 * Current angle value is reset to 0.
 */
void adxrs_capture_start(float scale);

/// Stop capture mode
void adxrs_capture_stop(void);

/** @brief Manually capture the next angle value
 *
 * \a scale is the value for the current capture and should be based on the
 * time since the previous capture.
 *
 * If scale is 0, captured valued is not used. It should be used to initialize
 * the capture.
 */
void adxrs_capture_manual(float scale);

/** @brief activate ADXRS calibration mode
 * @param activate if TRUE activate calibration mode
 */
void adxrs_calibration_mode(bool activate);

/** @brief if TRUE adxrs will integrate speed over time,
 * if FALSE it will not and angle will not change
 */
void adxrs_integrate(bool activate);

/** @brief Return TRUE if calibration is active, FALSE otherwise
 */
bool adxrs_get_calibration_mode(void);

/// Get current angle value
float adxrs_get_angle(void);

/// Set current angle value
void adxrs_set_angle(float angle);

/// Get current measured angular speed
float adxrs_get_speed(void);

/// Get current gyro offset
int16_t adxrs_get_offset(void);

/// Get current gyro offset sqsd
float adxrs_get_offset_sqsd(void);

//@}


/** @name Memory register map */
//@{

// Memory register addresses
#define ADXRS_REG_RATE1   0x00
#define ADXRS_REG_RATE0   0x01
#define ADXRS_REG_TEM1   0x02
#define ADXRS_REG_TEM0   0x03
#define ADXRS_REG_LOCST1   0x04
#define ADXRS_REG_LOCST0   0x05
#define ADXRS_REG_HICST1   0x06
#define ADXRS_REG_HICST0   0x07
#define ADXRS_REG_QUAD1   0x08
#define ADXRS_REG_QUAD0   0x09
#define ADXRS_REG_FAULT1   0x0A
#define ADXRS_REG_FAULT0   0x0B
#define ADXRS_REG_PID1   0x0C
#define ADXRS_REG_PID0   0x0D
#define ADXRS_REG_SN3   0x0E
#define ADXRS_REG_SN2   0x0F
#define ADXRS_REG_SN1   0x10
#define ADXRS_REG_SN0   0x11

// Memory register addresses, aliases
#define ADXRS_REG_RATE   ADXRS_REG_RATE1
#define ADXRS_REG_TEM   ADXRS_REG_TEM1
#define ADXRS_REG_LOCST  ADXRS_REG_LOCST1
#define ADXRS_REG_HICST  ADXRS_REG_HICST1
#define ADXRS_REG_QUAD  ADXRS_REG_QUAD1
#define ADXRS_REG_FAULT  ADXRS_REG_FAULT1
#define ADXRS_REG_PID  ADXRS_REG_PID1
#define ADXRS_REG_SN  ADXRS_REG_SN3

//@}


#endif
//@}
