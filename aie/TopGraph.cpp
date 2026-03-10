#include "TopGraph.h"

TopStencilGraph topStencil("hdiff");

#if defined(__AIESIM__) || defined(__X86SIM__)
int main() {
    topStencil.init();
    topStencil.run(1);
    topStencil.end();
    return 0;
}
#endif