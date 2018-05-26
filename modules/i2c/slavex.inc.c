/**
 * @internal
 * @file
 * @brief Include template code for i2c.c
 *
 * The X_(p,s) macro must be defined before including.
 * It is automatically undefined at the end of this file.
 */
#include <avarix.h>
#include <avr/interrupt.h>
#include <stddef.h>

#define I2CX(s) X_(I2C,s)
#define i2cX(s) X_(i2c,s)
#define TWIX X_(TWI,)
#define twiX(s) X_(TWI,s)

// declare i2cX singleton
i2cs_t i2cX();

void i2cX(_init)(void) {

  i2cX().state = I2CS_STATE_NONE;

  i2cX().reset_callback = NULL;
  i2cX().send_callback = NULL;
  i2cX().recv_callback = NULL;

  // initialize hardware
  TWIX.SLAVE.CTRLA = TWI_SLAVE_ENABLE_bm
    | TWI_SLAVE_APIEN_bm
    | TWI_SLAVE_DIEN_bm
    | TWI_SLAVE_PIEN_bm
    | ((I2CX(_INTLVL) << TWI_SLAVE_INTLVL_gp) & TWI_SLAVE_INTLVL_gm);
  TWIX.SLAVE.ADDR = I2CX(_ADDRESS) << 1;
}

void i2cX(s_register_reset_callback)(i2cs_reset_callback_t f) {
  INTLVL_DISABLE_BLOCK(I2CX(_INTLVL)) {
    i2cX().reset_callback = f;
  }
}

void i2cX(s_register_recv_callback)(i2cs_recv_callback_t f) {
  INTLVL_DISABLE_BLOCK(I2CX(_INTLVL)) {
    i2cX().recv_callback = f;
  }
}

void i2cX(s_register_send_callback)(i2cs_send_callback_t f) {
  INTLVL_DISABLE_BLOCK(I2CX(_INTLVL)) {
    i2cX().send_callback = f;
  }
}

// Interrupt handler
ISR(twiX(_TWIS_vect)) {

  i2cs_t *i2cs = &i2cX();

  uint8_t status = TWIX.SLAVE.STATUS;

  if(status & TWI_SLAVE_BUSERR_bm) {
    // bus error happened
    i2cs->state = I2CS_STATE_NONE;
    if(i2cs->reset_callback)
      i2cs->reset_callback();
    return;
  }

  else if(status & TWI_SLAVE_COLL_bm) {
    // collision happened
    i2cs->state = I2CS_STATE_NONE;
    if(i2cs->reset_callback)
      i2cs->reset_callback();
    return;
  }

  else if(status & TWI_SLAVE_APIF_bm) {
    // address / stop interrupt

    // check previous state, call recvd callback
    // if previous frame was a master-write
    if(i2cs->state == I2CS_STATE_WRITE) {
      if(i2cs->recv_callback) {
        i2cs->recv_callback(i2cs->recv_buffer, i2cs->recvd_bytes);
      }
    }

    if(status & TWI_SLAVE_AP_bm) {
      // valid address interrupt
      if(status & TWI_SLAVE_DIR_bm) {
        // master read operation

        i2cs->state = I2CS_STATE_READ;
        if(i2cs->send_callback) {

          i2cs->sent_bytes = 0;

          // ask user to provision send buffer
          int rsz = i2cs->send_callback(i2cs->send_buffer,
                                          sizeof(i2cs->send_buffer));
          if(rsz > 0) {
            // user got some data to send
            i2cs->bytes_to_send = MIN(rsz,I2CS_SEND_BUFFER_SIZE);
            // ACK
            TWIX.SLAVE.CTRLB = TWI_SLAVE_CMD_RESPONSE_gc;
            return;
          }
          i2cs->bytes_to_send = 0;
        }

        // NACK, refuse read
        TWIX.SLAVE.CTRLB = TWI_SLAVE_ACKACT_bm | TWI_SLAVE_CMD_RESPONSE_gc;
        return;
      }
      else {
        // master write operation
        i2cs->state = I2CS_STATE_WRITE;

        // clear recv buffer
        i2cs->recvd_bytes = 0;
        // confirm operation
        TWIX.SLAVE.CTRLB = TWI_SLAVE_CMD_RESPONSE_gc;
      }
      return;
    }
    else {
      // STOP condition interrupt
      TWIX.SLAVE.CTRLB = TWI_SLAVE_CMD_COMPTRANS_gc;

      i2cs->state = I2CS_STATE_NONE;
      if(i2cs->reset_callback)
        i2cs->reset_callback();
      return;
    }
  }

  else if(status & TWI_SLAVE_DIF_bm) {
    // data interruption
    if(status & TWI_SLAVE_DIR_bm) {
      // master read operation

      if(i2cs->sent_bytes > 0 && (status & TWI_SLAVE_RXACK_bm)) {
        // previous byte was NACKed by master
        TWIX.SLAVE.CTRLB = TWI_SLAVE_CMD_COMPTRANS_gc;
        i2cs->state = I2CS_STATE_NONE;
        return;
      }

      if(i2cs->sent_bytes < i2cs->bytes_to_send)  {
        // push byte
        uint8_t byte = i2cs->send_buffer[i2cs->sent_bytes++];
        TWIX.SLAVE.DATA = byte;

        // ACK read, continue transmission
        TWIX.SLAVE.CTRLB = TWI_SLAVE_CMD_RESPONSE_gc;
      }
      else {
        // ACK read, end transmission
        TWIX.SLAVE.CTRLB = TWI_SLAVE_CMD_COMPTRANS_gc;
      }

      return;
    }
    else {
      // master write operation
      uint8_t byte = TWIX.SLAVE.DATA;

      if(i2cs->recvd_bytes < I2CS_RECV_BUFFER_SIZE) {
        i2cs->recv_buffer[i2cs->recvd_bytes++] = byte;

        // ACK write
        TWIX.SLAVE.CTRLB = TWI_SLAVE_CMD_RESPONSE_gc;
      }
      else {
        // NACK write, buffer is full
        TWIX.SLAVE.CTRLB = TWI_SLAVE_ACKACT_bm | TWI_SLAVE_CMD_COMPTRANS_gc;
      }

      return;
    }
  }

  // we encounter an unmanaged case
  i2cs->state = I2CS_STATE_NONE;
  i2cs->recvd_bytes = 0;
  i2cs->sent_bytes = 0;
  // reset client-side state
  if(i2cs->reset_callback)
    i2cs->reset_callback();
}

#undef I2CX
#undef i2cX
#undef TWIX
#undef twiX
#undef X_
