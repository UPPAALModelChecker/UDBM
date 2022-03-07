#include "dbm/fed.h"

void fed_crash(cindex_t dim)
{
    dbm::fed_t fed(dim);
    fed.setZero();
    fed.nil();
}

int main(){
    fed_crash(10);
    fed_crash(15);
    fed_crash(16);
    fed_crash(17);
}