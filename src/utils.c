
#include "utils.h"
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>
#include <ctype.h>
#include <pthread.h>
#include <stdlib.h>
/** utils  **/
/* Given the filename, return the absolute path as an SDS string, or NULL
 * if it fails for some reason. Note that "filename" may be an absolute path
 * already, this will be detected and handled correctly.
 *
 * The function does not try to normalize everything, but only the obvious
 * case of one or more "../" appearing at the start of "filename"
 * relative path. */
sds getAbsolutePath(char *filename) {
    char cwd[1024];
    sds abspath;
    sds relpath = sdsnew(filename);

    relpath = sdstrim(relpath," \r\n\t");
    if (relpath[0] == '/') return relpath; /* Path is already absolute. */

    /* If path is relative, join cwd and relative path. */
    if (getcwd(cwd,sizeof(cwd)) == NULL) {
        sdsfree(relpath);
        return NULL;
    }
    abspath = sdsnew(cwd);
    if (sdslen(abspath) && abspath[sdslen(abspath)-1] != '/')
        abspath = sdscat(abspath,"/");

    /* At this point we have the current path always ending with "/", and
     * the trimmed relative path. Try to normalize the obvious case of
     * trailing ../ elements at the start of the path.
     *
     * For every "../" we find in the filename, we remove it and also remove
     * the last element of the cwd, unless the current cwd is "/". */
    while (sdslen(relpath) >= 3 &&
           relpath[0] == '.' && relpath[1] == '.' && relpath[2] == '/')
    {
        sdsrange(relpath,3,-1);
        if (sdslen(abspath) > 1) {
            char *p = abspath + sdslen(abspath)-2;
            int trimlen = 1;

            while(*p != '/') {
                p--;
                trimlen++;
            }
            sdsrange(abspath,0,-(trimlen+1));
        }
    }

    /* Finally glue the two parts together. */
    abspath = sdscatsds(abspath,relpath);
    sdsfree(relpath);
    return abspath;
}


bool is_blank(const char *s) {
    // 如果字符串指针为 nullptr，则认为该字符串为空白
    if (s == NULL) {
        return true;
    }
    
    // 遍历字符串的每一个字符
    while (*s != '\0') {
        // 如果当前字符不是空白字符，则返回 false
        if (!isspace((unsigned char)*s)) {
            return false;
        }
        s++; // 移动到下一个字符
    }
    
    // 如果所有字符都是空白字符，则返回 true
    return true;
}

// 检查并打印错误信息
void minidb_assert(int condition, const char *message, ...) {
    if (!condition) {
        va_list args;
        va_start(args, message);
        fprintf(stderr, "Assertion failed: ");
        vfprintf(stderr, message, args);
        va_end(args);
        exit(EXIT_FAILURE);
    }
}