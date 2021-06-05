#ifndef UTILS_H
#define UTILS_H

#include <stdint.h>

void init_crc(void);
uint32_t compute_device_id(void);

#define REG_FIELD(field, value) ((value) << field ## _Pos)
#define REG_FIELD_GET(value, field) (((value) & field ## _Msk) >> field ## _Pos)

#endif
