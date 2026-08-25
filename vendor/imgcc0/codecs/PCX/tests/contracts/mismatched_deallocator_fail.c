/* PCX89 intentionally has no dynamic deallocator contract.
 * This negative compile test verifies the removed legacy allocation API stays absent.
 */
#include "../../pcx.h"

int main(void)
{
    void *p;
    p = pcx_malloc(16U);
    (void)p;
    return 0;
}
