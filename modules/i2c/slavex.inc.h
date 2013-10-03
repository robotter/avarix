/** @defgroup i2c I2C
 * @brief I2C module
 */
//@{
/**
 * @file
 * @brief I2C definitions
 */
#define i2cX(s) X_(i2c,s)


/* I2cX Register callback */
void i2cX(s_register_callback)( void (*process_data_function)(void));

// I2cX singleton
extern i2cs_t i2cX();

#undef i2cX
#undef X_
