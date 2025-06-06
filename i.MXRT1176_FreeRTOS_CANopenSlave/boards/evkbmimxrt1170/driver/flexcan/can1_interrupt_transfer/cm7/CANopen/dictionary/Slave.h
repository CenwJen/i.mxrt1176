
/* File generated by gen_cfile.py. Should not be modified. */

#ifndef SLAVE_H
#define SLAVE_H

#include "data.h"

/* Prototypes of function provided by object dictionnary */
UNS32 Slave_valueRangeTest (UNS8 typeValue, void * value);
const indextable * Slave_scanIndexOD (CO_Data *d, UNS16 wIndex, UNS32 * errorCode);

/* Master node data struct */
extern CO_Data Slave_Data;
extern UNS8 ZONE_ONE;		/* Mapped at index 0x2000, subindex 0x00*/
extern UNS8 ZONE_TWO;		/* Mapped at index 0x2001, subindex 0x00*/
extern UNS8 ZONE_THREE;		/* Mapped at index 0x2002, subindex 0x00*/

#endif // SLAVE_H
