/** @defgroup i2c I2C
 * @brief I2C module
 */
//@{
/**
 * @file
 * @brief I2C definitions
 */
#ifndef SLAVEX_INC_H__
#define SLAVEX_INC_H__

#define i2cX(s) X_(i2c,s)
/*! \brief Initialize the I2C module.
 *
 *  Enables interrupts on address recognition and data available.
 *  Remember to enable interrupts globally from the main application.
 *
 *  \param address    Slave address for this module.
 *  \param process_data_function  Pointer to the function that handles incoming data.
 */
void i2cX(s_init)(i2cs_t *p ,
                  uint8_t address,
                  void (*process_data_function)(void));

#undef i2cX
#endif//SLAVEX_INC_H__
