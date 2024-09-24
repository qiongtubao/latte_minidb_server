#include "page.h"

Page* pageCreate() {
    Page* page = zmalloc(BP_PAGE_SIZE - sizeof(PageNum));
    return page;
}