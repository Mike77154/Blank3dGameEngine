#ifndef CCS_CONTACT_H
#define CCS_CONTACT_H

#include "ccs_math.h"

/*
    ccs_contact
    -----------
    Contacto mínimo: normal (A->B) + penetración.

    Nota:
    - Normal idealmente unit length en Q16.16 (aprox).
    - Penetración en la misma escala fixed.
*/

typedef struct {
    ccs_vec3  normal;
    ccs_fixed penetration;
} ccs_contact;

#endif /* CCS_CONTACT_H */
