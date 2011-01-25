
void i2c_init(unsigned i2c_base, unsigned speed);
int i2c_read(unsigned i2c_base, uint8_t chip, uint addr, int alen, uint8_t *buf, int len);
int i2c_write(unsigned i2c_base, uint8_t chip, uint addr, int alen, uint8_t *buf, int len);
