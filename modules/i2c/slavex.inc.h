/** @defgroup i2c I2C
 * @brief I2C module
 */
//@{
/**
 * @file
 * @brief I2C definitions
 */
#define i2cX(s) X_(i2c,s)

// I2cX singleton
extern i2cs_t i2cX();

void i2cX(s_register_send_callback)(i2cs_send_callback_t f);

void i2cX(s_register_recv_callback)(i2cs_recv_callback_t f);

void i2cX(s_register_reset_callback)(i2cs_reset_callback_t f);

#undef i2cX
#undef X_
