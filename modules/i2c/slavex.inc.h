/** @defgroup i2c I2C
 * @brief I2C module
 */
//@{
/**
 * @file
 * @brief I2C slave definitions
 */
#define i2cX(s) X_(i2c,s)

// I2cX singleton
extern i2cs_t i2cX();

/** @brief Register f to be called whenever a master-read operation was requested and user need
 * to provision the send buffer */
void i2cX(s_register_prepare_send_callback)(i2cs_prepare_send_callback_t f);

/** @brief Register f to be called whenever a master-write operation as finished */
void i2cX(s_register_recv_callback)(i2cs_recv_callback_t f);

/** @brief Register f to be called whenever the i2c transaction was terminated (on STOP or ERROR) */
void i2cX(s_register_reset_callback)(i2cs_reset_callback_t f);

#undef i2cX
#undef X_
