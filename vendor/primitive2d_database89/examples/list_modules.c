#include "primitive2d_database89.h"
#include <stdio.h>
int main(void){unsigned int i;const P2D89_ModuleInfo *m;for(i=0U;i<p2d89_submodule_count();++i){m=p2d89_submodule_at(i);printf("%s/%s: %u\n",m->module_name,m->submodule_name,m->shape_count);}return 0;}
