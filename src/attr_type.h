#ifndef __LATTE_DEFAULT_ATTR_TYPE_H
#define __LATTE_DEFAULT_ATTR_TYPE_H


typedef enum  AttrType
{
  UNDEFINED,
  CHARS,     ///< 字符串类型
  INTS,      ///< 整数类型(4字节)
  FLOATS,    ///< 浮点数类型(4字节)
  BOOLEANS,  ///< boolean类型，当前不是由parser解析出来的，是程序内部使用的
  MAXTYPE,   ///< 请在 UNDEFINED 与 MAXTYPE 之间增加新类型
} AttrType;

#endif