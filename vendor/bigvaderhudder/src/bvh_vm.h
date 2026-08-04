#ifndef BVH_VM_H_INCLUDED
#define BVH_VM_H_INCLUDED

#include "bvh_bytecode.h"

int bvh_vm_exec(const BVH_Bytecode *bc, const BVH_RuntimeCallbacks *callbacks, void *user, BVH_Error *err);

#endif
