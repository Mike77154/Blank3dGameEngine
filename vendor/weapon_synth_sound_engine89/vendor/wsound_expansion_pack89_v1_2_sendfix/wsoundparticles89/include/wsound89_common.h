#ifndef WSOUND89_COMMON_H
#define WSOUND89_COMMON_H
#ifdef __cplusplus
extern "C" {
#endif
typedef signed char wsound89_i8;
typedef unsigned char wsound89_u8;
typedef signed short wsound89_i16;
typedef unsigned short wsound89_u16;
typedef signed int wsound89_i32;
typedef unsigned int wsound89_u32;
#define WSOUND89_Q15_ONE 32767
#define WSOUND89_Q16_ONE 65536U
typedef enum wsound89_result_e { WSOUND89_OK=0, WSOUND89_EINVAL=-1, WSOUND89_ECAPACITY=-2, WSOUND89_ESTATE=-3 } wsound89_result;
#ifdef __cplusplus
}
#endif
#endif
