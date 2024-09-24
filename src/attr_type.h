#ifndef __LATTE_DEFAULT_ATTR_TYPE_H
#define __LATTE_DEFAULT_ATTR_TYPE_H

#include "value/value.h" 

//TODO attrtype 未来是否可以转换成value_type_enum使用呢？

typedef enum  attr_type_enum
{
  UNDEFINED,
  CHARS,     ///< 字符串类型
  INTS,      ///< 整数类型(4字节)
  FLOATS,    ///< 浮点数类型(4字节)
  BOOLEANS,  ///< boolean类型，当前不是由parser解析出来的，是程序内部使用的
  MAXTYPE,   ///< 请在 UNDEFINED 与 MAXTYPE 之间增加新类型
} attr_type_enum;
const char* attr_type_to_str(attr_type_enum type);
attr_type_enum attr_type_from_str(const char *s);
value_type_enum attr_type_to_value_type(attr_type_enum type);

#endif