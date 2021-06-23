/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkParseType.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#ifndef svtkParseType_h
#define svtkParseType_h

/**
 * The parser identifies SVTK types with 32-bit hexadecimal numbers:
 *
 * - One byte is for the base type.
 * - One byte is indirection i.e. & and * and "* const"
 * - One byte is for qualifiers like const and static.
 * - The final byte is reserved.
 *
 * There is some type information that cannot be stored within
 * this bitfield.  This info falls into three categories:
 *
 * 1) Function pointers are stored in a FunctionInfo struct.
 *    However, if the type is SVTK_PARSE_FUNCTION with no POINTER,
 *    it is guaranteed to be "void func(void *)" which is the
 *    old SVTK-style callback.
 *
 * 2) Multi-dimensional arrays are stored in a char *[MAX_ARRAY_DIMS]
 *    array with a NULL pointer indicating there are no more brackets.
 *    If the type is a pointer and the first value is not NULL, then
 *    that value gives the array size for that pointer.  The reason
 *    that "char *" is used is because the sizes might be template
 *    parameters or constants defined elsewhere.  However, most often
 *    the sizes are integer literals, and the first size will be
 *    stored as an int in ArgCounts.
 *
 * 3) The ID for SVTK_PARSE_OBJECT is stored in ArgClasses.
 *
 */

/**
 * Mask for removing everything but the base type
 */
#define SVTK_PARSE_BASE_TYPE 0x000000FF

/**
 * Mask for checking signed/unsigned
 */
#define SVTK_PARSE_UNSIGNED 0x00000010

/**
 * Mask for pointers and references
 */
#define SVTK_PARSE_INDIRECT 0x0000FF00

/**
 * Qualifiers
 */
#define SVTK_PARSE_QUALIFIER 0x00FF0000
#define SVTK_PARSE_CONST 0x00010000
#define SVTK_PARSE_STATIC 0x00020000
#define SVTK_PARSE_VIRTUAL 0x00040000
#define SVTK_PARSE_EXPLICIT 0x00080000
#define SVTK_PARSE_MUTABLE 0x00100000
#define SVTK_PARSE_VOLATILE 0x00200000
#define SVTK_PARSE_RVALUE 0x00400000
#define SVTK_PARSE_THREAD_LOCAL 0x00800000

/**
 * Attributes (used for hints)
 */
#define SVTK_PARSE_ATTRIBUTES 0x07000000
#define SVTK_PARSE_NEWINSTANCE 0x01000000
#define SVTK_PARSE_ZEROCOPY 0x02000000
#define SVTK_PARSE_WRAPEXCLUDE 0x04000000

/**
 * Special
 */
#define SVTK_PARSE_SPECIALS 0x70000000
#define SVTK_PARSE_TYPEDEF 0x10000000
#define SVTK_PARSE_FRIEND 0x20000000
#define SVTK_PARSE_PACK 0x40000000

/**
 * Mask for removing qualifiers
 */
#define SVTK_PARSE_QUALIFIED_TYPE 0x03FFFFFF
#define SVTK_PARSE_UNQUALIFIED_TYPE 0x0000FFFF

/**
 * Indirection, contained in SVTK_PARSE_INDIRECT
 *
 * Indirection of types works as follows:
 * type **(**&val[n])[m]
 * Pointers on the left, arrays on the right,
 * and optionally a set of parentheses and a ref.
 *
 * The 'type' may be preceded or followed by const,
 * which is handled by the SVTK_PARSE_CONST flag.
 *
 * The array dimensionality and sizes is stored
 * elsewhere, it isn't stored in the bitfield.
 *
 * The leftmost [] is converted to a pointer, unless
 * it is outside the parenthesis.
 * So "type val[n][m]"  becomes  "type (*val)[m]",
 * these two types are identical in C and C++.
 *
 * Any pointer can be followed by const, and any pointer
 * can be preceded by a parenthesis. However, you will
 * never see a parenthesis anywhere except for just before
 * the leftmost pointer.
 *
 * These are good: "(*val)[n]", "**(*val)[n]", "(*&val)[n]"
 * Not so good: "(**val)[n]" (is actually like (*val)[][n])
 *
 * The Ref needs 1 bit total, and each pointer needs 2 bits:
 *
 *  0 = nothing
 *  1 = '*'       = SVTK_PARSE_POINTER
 *  2 = '[]'      = SVTK_PARSE_ARRAY
 *  3 = '* const' = SVTK_PARSE_CONST_POINTER
 *
 * The SVTK_PARSE_ARRAY flag means "this pointer is actually
 * the first bracket in a multi-dimensional array" with the array
 * info stored separately.
 */
#define SVTK_PARSE_BAD_INDIRECT 0xFF00
#define SVTK_PARSE_POINTER_MASK 0xFE00
#define SVTK_PARSE_POINTER_LOWMASK 0x0600
#define SVTK_PARSE_REF 0x0100
#define SVTK_PARSE_POINTER 0x0200
#define SVTK_PARSE_POINTER_REF 0x0300
#define SVTK_PARSE_ARRAY 0x0400
#define SVTK_PARSE_ARRAY_REF 0x0500
#define SVTK_PARSE_CONST_POINTER 0x0600
#define SVTK_PARSE_CONST_POINTER_REF 0x0700
#define SVTK_PARSE_POINTER_POINTER 0x0A00
#define SVTK_PARSE_POINTER_POINTER_REF 0x0B00
#define SVTK_PARSE_POINTER_CONST_POINTER 0x0E00

/**
 * Basic types contained in SVTK_PARSE_BASE_TYPE
 *
 * The lowest two hex digits describe the basic type,
 * where bit 0x10 is used to indicate unsigned types,
 * value 0x8 is used for unrecognized types, and
 * value 0x9 is used for types that start with "svtk".
 *
 * The bit 0x10 is reserved for "unsigned", and it
 * may only be present in unsigned types.
 *
 * Do not rearrange these types, they are hard-coded
 * into the hints file.
 */
#define SVTK_PARSE_FLOAT 0x01
#define SVTK_PARSE_VOID 0x02
#define SVTK_PARSE_CHAR 0x03
#define SVTK_PARSE_UNSIGNED_CHAR 0x13
#define SVTK_PARSE_INT 0x04
#define SVTK_PARSE_UNSIGNED_INT 0x14
#define SVTK_PARSE_SHORT 0x05
#define SVTK_PARSE_UNSIGNED_SHORT 0x15
#define SVTK_PARSE_LONG 0x06
#define SVTK_PARSE_UNSIGNED_LONG 0x16
#define SVTK_PARSE_DOUBLE 0x07
#define SVTK_PARSE_UNKNOWN 0x08
#define SVTK_PARSE_OBJECT 0x09
#define SVTK_PARSE_LONG_LONG 0x0B
#define SVTK_PARSE_UNSIGNED_LONG_LONG 0x1B
#define SVTK_PARSE___INT64 0x0C
#define SVTK_PARSE_UNSIGNED___INT64 0x1C
#define SVTK_PARSE_SIGNED_CHAR 0x0D
#define SVTK_PARSE_BOOL 0x0E
#define SVTK_PARSE_SSIZE_T 0x0F
#define SVTK_PARSE_SIZE_T 0x1F
#define SVTK_PARSE_STRING 0x21
#define SVTK_PARSE_UNICODE_STRING 0x22
#define SVTK_PARSE_OSTREAM 0x23
#define SVTK_PARSE_ISTREAM 0x24
#define SVTK_PARSE_FUNCTION 0x25
#define SVTK_PARSE_QOBJECT 0x26
#define SVTK_PARSE_LONG_DOUBLE 0x27
#define SVTK_PARSE_WCHAR_T 0x28
#define SVTK_PARSE_CHAR16_T 0x29
#define SVTK_PARSE_CHAR32_T 0x2A
#define SVTK_PARSE_NULLPTR_T 0x2B

/**
 * Basic pointer types
 */
#define SVTK_PARSE_FLOAT_PTR 0x201
#define SVTK_PARSE_VOID_PTR 0x202
#define SVTK_PARSE_CHAR_PTR 0x203
#define SVTK_PARSE_UNSIGNED_CHAR_PTR 0x213
#define SVTK_PARSE_INT_PTR 0x204
#define SVTK_PARSE_UNSIGNED_INT_PTR 0x214
#define SVTK_PARSE_SHORT_PTR 0x205
#define SVTK_PARSE_UNSIGNED_SHORT_PTR 0x215
#define SVTK_PARSE_LONG_PTR 0x206
#define SVTK_PARSE_UNSIGNED_LONG_PTR 0x216
#define SVTK_PARSE_DOUBLE_PTR 0x207
#define SVTK_PARSE_UNKNOWN_PTR 0x208
#define SVTK_PARSE_OBJECT_PTR 0x209
#define SVTK_PARSE_LONG_LONG_PTR 0x20B
#define SVTK_PARSE_UNSIGNED_LONG_LONG_PTR 0x21B
#define SVTK_PARSE___INT64_PTR 0x20C
#define SVTK_PARSE_UNSIGNED___INT64_PTR 0x21C
#define SVTK_PARSE_SIGNED_CHAR_PTR 0x20D
#define SVTK_PARSE_BOOL_PTR 0x20E
#define SVTK_PARSE_SSIZE_T_PTR 0x20F
#define SVTK_PARSE_SIZE_T_PTR 0x21F
#define SVTK_PARSE_STRING_PTR 0x221
#define SVTK_PARSE_UNICODE_STRING_PTR 0x222
#define SVTK_PARSE_OSTREAM_PTR 0x223
#define SVTK_PARSE_ISTREAM_PTR 0x224
#define SVTK_PARSE_FUNCTION_PTR 0x225
#define SVTK_PARSE_QOBJECT_PTR 0x226
#define SVTK_PARSE_LONG_DOUBLE_PTR 0x227
#define SVTK_PARSE_WCHAR_T_PTR 0x228
#define SVTK_PARSE_CHAR16_T_PTR 0x229
#define SVTK_PARSE_CHAR32_T_PTR 0x22A
#define SVTK_PARSE_NULLPTR_T_PTR 0x22B

/**
 * Basic reference types
 */
#define SVTK_PARSE_FLOAT_REF 0x101
#define SVTK_PARSE_VOID_REF 0x102
#define SVTK_PARSE_CHAR_REF 0x103
#define SVTK_PARSE_UNSIGNED_CHAR_REF 0x113
#define SVTK_PARSE_INT_REF 0x104
#define SVTK_PARSE_UNSIGNED_INT_REF 0x114
#define SVTK_PARSE_SHORT_REF 0x105
#define SVTK_PARSE_UNSIGNED_SHORT_REF 0x115
#define SVTK_PARSE_LONG_REF 0x106
#define SVTK_PARSE_UNSIGNED_LONG_REF 0x116
#define SVTK_PARSE_DOUBLE_REF 0x107
#define SVTK_PARSE_UNKNOWN_REF 0x108
#define SVTK_PARSE_OBJECT_REF 0x109
#define SVTK_PARSE_LONG_LONG_REF 0x10B
#define SVTK_PARSE_UNSIGNED_LONG_LONG_REF 0x11B
#define SVTK_PARSE___INT64_REF 0x10C
#define SVTK_PARSE_UNSIGNED___INT64_REF 0x11C
#define SVTK_PARSE_SIGNED_CHAR_REF 0x10D
#define SVTK_PARSE_BOOL_REF 0x10E
#define SVTK_PARSE_SSIZE_T_REF 0x10F
#define SVTK_PARSE_SIZE_T_REF 0x11F
#define SVTK_PARSE_STRING_REF 0x121
#define SVTK_PARSE_UNICODE_STRING_REF 0x122
#define SVTK_PARSE_OSTREAM_REF 0x123
#define SVTK_PARSE_ISTREAM_REF 0x124
#define SVTK_PARSE_QOBJECT_REF 0x126
#define SVTK_PARSE_LONG_DOUBLE_REF 0x127
#define SVTK_PARSE_WCHAR_T_REF 0x128
#define SVTK_PARSE_CHAR16_T_REF 0x129
#define SVTK_PARSE_CHAR32_T_REF 0x12A
#define SVTK_PARSE_NULLPTR_T_REF 0x12B

/**
 * For backwards compatibility
 */
#ifndef SVTK_PARSE_LEGACY_REMOVE
#define SVTK_PARSE_SVTK_OBJECT SVTK_PARSE_OBJECT
#define SVTK_PARSE_SVTK_OBJECT_PTR SVTK_PARSE_OBJECT_PTR
#define SVTK_PARSE_SVTK_OBJECT_REF SVTK_PARSE_OBJECT_REF
#define SVTK_PARSE_ID_TYPE 0x0A
#define SVTK_PARSE_UNSIGNED_ID_TYPE 0x1A
#define SVTK_PARSE_ID_TYPE_PTR 0x20A
#define SVTK_PARSE_UNSIGNED_ID_TYPE_PTR 0x21A
#define SVTK_PARSE_ID_TYPE_REF 0x10A
#define SVTK_PARSE_UNSIGNED_ID_TYPE_REF 0x11A
#endif

#endif
/* SVTK-HeaderTest-Exclude: svtkParseType.h */
