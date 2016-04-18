/*

This a LSS Configurator to load pin.

Copyright (C): Héctor ASCORBE and Sergio RODRÍGUEZ.

*/
#pragma once

#ifdef USE_XENO
#define eprintf(...)
#else
#define eprintf(...) printf (__VA_ARGS__)
#endif

#define SLSS_ADRESS	0x7E4
#define MLSS_ADRESS	0x7E5

#define SDELAY_OFF		0
#define SDELAY_FIRST 	1
#define SDELAY_SECOND 	2

#define LSS_WAITING_MODE 0
#define LSS_CONFIGURATION_MODE 1

#define LSS_SM_GLOBAL 				 4
#define LSS_SM_SELECTIVE_VENDOR 	64
#define LSS_SM_SELECTIVE_PRODUCT 	65
#define LSS_SM_SELECTIVE_REVISION 	66
#define LSS_SM_SELECTIVE_SERIAL 	67
#define LSS_SM_SELECTIVE_RESP 		68
#define LSS_CONF_NODE_ID 			17
#define LSS_CONF_BIT_TIMING 		19
#define LSS_CONF_ACT_BIT_TIMING 	21
#define LSS_CONF_STORE 				23
#define LSS_INQ_VENDOR_ID 		90
#define LSS_INQ_PRODUCT_CODE 	91
#define LSS_INQ_REV_NUMBER 		92
#define LSS_INQ_SERIAL_NUMBER 	93
#define LSS_INQ_NODE_ID 		94
#define LSS_IDENT_REMOTE_VENDOR 		70
#define LSS_IDENT_REMOTE_PRODUCT 		71
#define LSS_IDENT_REMOTE_REV_LOW 		72
#define LSS_IDENT_REMOTE_REV_HIGH 		73
#define LSS_IDENT_REMOTE_SERIAL_LOW 	74
#define LSS_IDENT_REMOTE_SERIAL_HIGH 	75
#define LSS_IDENT_REMOTE_NON_CONF 		76
#define LSS_IDENT_SLAVE 				79
#define LSS_IDENT_NON_CONF_SLAVE 		80
#define LSS_IDENT_FASTSCAN				81

#define LSS_FS_RESET	0
#define LSS_FS_PROCESSING 1
#define LSS_FS_CONFIRMATION 2

#include "canfestival.h"
