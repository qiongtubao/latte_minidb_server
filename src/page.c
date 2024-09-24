#include "page.h"

page_t* page_new() {
    page_t* page = zmalloc(BP_PAGE_SIZE - sizeof(PageNum));
    return page;
}