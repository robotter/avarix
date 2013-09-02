/**
 * @internal
 * @file
 * @brief Include template code for i2c.c
 *
 * The X_(p,s) macro must be defined before including.
 * It is automatically undefined at the end of this file.
 */
#include <avr/interrupt.h>

#define I2CX(s) X_(I2C,s)
#define i2cX(s) X_(i2c,s)
#define i2cs X_(i2cs,)
#define TWIX X_(TWI,)
#define twiX(s) X_(TWI,s)

i2cs_t *i2csX;

void i2cX(_init)(void)
{
}


/*! \brief Initialize the I2C module. 
 */
void i2cX(s_init)(i2cs_t *p,
                  uint8_t address,
                  void (*process_data_function)(void)) {
  i2csX = p;
  i2csX->process_data = process_data_function;
  i2csX->bytes_received = 0;
  i2csX->bytes_transmit = 0;
  i2csX->status = I2CS_STATUS_READY;
  i2csX->result = I2CS_RESULT_UNKNOWN;

  TWIX.SLAVE.CTRLA = (I2C_INTLVL<<TWI_SLAVE_INTLVL_gp) |
                     TWI_SLAVE_DIEN_bm |
                     TWI_SLAVE_APIEN_bm |
                     TWI_SLAVE_ENABLE_bm;
  TWIX.SLAVE.ADDR = (address<<1);
}

/*! \brief I2C transaction finished function.
 *
 *  Prepares module for new transaction.
 *
 *  \param i2c    The i2cs_t struct instance.
 *  \param result The result of the transaction.
 */
void i2cX(_transaction_finished(uint8_t result)) {
  i2csX->result = result;
  i2csX->status = I2CS_STATUS_READY;
}

/*! \brief i2c slave transmit interrupt handler.
 *
 *  Handles i2c slave transmit transactions and responses.
 *
 *  \param i2c The i2cs_t struct instance.
 */
void i2cX(_transmit_handler(void)) {
  /* If NACK, slave transmit transaction finished. */
  if ((i2csX->bytes_transmit > 0) && (TWIX.SLAVE.STATUS &
                                 TWI_SLAVE_RXACK_bm)) {

    TWIX.SLAVE.CTRLB = TWI_SLAVE_CMD_COMPTRANS_gc;
    i2cX(_transaction_finished(I2CS_RESULT_OK));
  }
  /* If ACK, master expects more data. */
  else {
    if (i2csX->bytes_transmit < I2CS_SEND_BUFFER_SIZE) {
      uint8_t data = i2csX->transmit_data[i2csX->bytes_transmit];
      TWIX.SLAVE.DATA = data;
      i2csX->bytes_transmit++;

      /* Send data, wait for data interrupt. */
      TWIX.SLAVE.CTRLB = TWI_SLAVE_CMD_RESPONSE_gc;
    }
    /* If buffer overflow. */
    else {
      TWIX.SLAVE.CTRLB = TWI_SLAVE_CMD_COMPTRANS_gc;
      i2cX(_transaction_finished(I2CS_RESULT_BUFFER_OVERFLOW));
    }
  }
}

/*! \brief TWI slave recieve interrupt handler.
 *
 *  Handles TWI slave recieve transactions and responses.
 *
 *  \param twi The TWI_Slave_t struct instance.
 */
void i2cX(_receive_handler(void)) {
  /* Enable stop interrupt. */
  uint8_t currentCtrlA = TWIX.SLAVE.CTRLA;
  TWIX.SLAVE.CTRLA = currentCtrlA | TWI_SLAVE_PIEN_bm;

  /* If free space in buffer. */
  if (i2csX->bytes_received < I2CS_RECEIVE_BUFFER_SIZE) {
    /* Fetch data */
    uint8_t data = TWIX.SLAVE.DATA;
    i2csX->received_data[i2csX->bytes_received] = data;

    /* Process data. */
    if(i2csX->process_data != NULL)
      i2csX->process_data();

    i2csX->bytes_received++;

    TWIX.SLAVE.CTRLB = TWI_SLAVE_CMD_RESPONSE_gc;
  }
  /* If buffer overflow, send NACK and wait for next START. Set
   * result buffer overflow.
   */
  else {
    TWIX.SLAVE.CTRLB = TWI_SLAVE_ACKACT_bm |
      TWI_SLAVE_CMD_COMPTRANS_gc;
    i2cX(_transaction_finished(I2CS_RESULT_BUFFER_OVERFLOW));
  }
}

/*! \brief TWI address match interrupt handler.
 *
 *  Prepares TWI module for transaction when an address match occures.
 *
 *  \param twi The TWI_Slave_t struct instance.
 */
void i2cX(_address_match_handler(void)) {
  i2csX->status = I2CS_STATUS_BUSY;
  i2csX->result = I2CS_RESULT_UNKNOWN;
  i2csX->bytes_received = 0;
  i2csX->bytes_transmit = 0;

  /* Disable stop interrupt. */
  uint8_t currentCtrlA = TWIX.SLAVE.CTRLA;
  TWIX.SLAVE.CTRLA = currentCtrlA & ~TWI_SLAVE_PIEN_bm;


  /* Send ACK, wait for data interrupt. */
  TWIX.SLAVE.CTRLB = TWI_SLAVE_CMD_RESPONSE_gc;
}


/*! \brief I2C stop condition interrupt handler.
 *
 *  \param i2c The i2cs_t struct instance.
 */
void i2cX(_stop_handler(void)) {
  /* Disable stop interrupt. */
  uint8_t currentCtrlA = TWIX.SLAVE.CTRLA;
  TWIX.SLAVE.CTRLA = currentCtrlA & ~TWI_SLAVE_PIEN_bm;

  /* Clear APIF, according to flowchart don't ACK or NACK */
  uint8_t currentStatus = TWIX.SLAVE.STATUS;
  TWIX.SLAVE.STATUS = currentStatus | TWI_SLAVE_APIF_bm;

  i2cX(_transaction_finished(I2CS_RESULT_OK));

}

/*! \brief TWI data interrupt handler.
 *
 *  Calls the appropriate slave receive or transmit handler.
 *
 *  \param twi The TWI_Slave_t struct instance.
 */
void i2cX(_data_handler(void)) {
  if (TWIX.SLAVE.STATUS & TWI_SLAVE_DIR_bm) {
    i2cX(_transmit_handler());
  } else {
    i2cX(_receive_handler());
  }
}

/// Interrupt handler 
ISR(twiX(_TWIS_vect)) {
  uint8_t current_status = TWIX.SLAVE.STATUS;

  /* If bus error. */
  if (current_status & TWI_SLAVE_BUSERR_bm) {
    i2csX->bytes_received = 0;
    i2csX->bytes_transmit = 0;
    i2csX->result = I2CS_RESULT_BUS_ERROR;
    i2csX->status = I2CS_STATUS_READY;
  }

  /* If transmit collision. */
  else if (current_status & TWI_SLAVE_COLL_bm) {
    i2csX->bytes_received = 0;
    i2csX->bytes_transmit = 0;
    i2csX->result = I2CS_RESULT_TRANSMIT_COLLISION;
    i2csX->status = I2CS_STATUS_READY;
  }

  /* If address match. */
  else if ((current_status & TWI_SLAVE_APIF_bm) &&
           (current_status & TWI_SLAVE_AP_bm)) {
    i2cX(_address_match_handler());
  }

  /* If stop (only enabled through slave receive transaction). */
  else if (current_status & TWI_SLAVE_APIF_bm) {
    i2cX(_stop_handler());
  }

  /* If data interrupt. */
  else if (current_status & TWI_SLAVE_DIF_bm) {
    i2cX(_data_handler());
  }

  /* If unexpected state. */
  else {
    i2cX(_transaction_finished(I2CS_RESULT_FAIL));
  }
}

#undef I2CX
#undef i2cX
#undef i2csX
#undef TWIX
#undef twiX
#undef X_
