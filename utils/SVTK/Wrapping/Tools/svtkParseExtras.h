/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkParseExtras.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/*-------------------------------------------------------------------------
  Copyright (c) 2011 David Gobbi.

  Contributed to the VisualizationToolkit by the author in May 2011
  under the terms of the Visualization Toolkit 2008 copyright.
-------------------------------------------------------------------------*/

/**
 * This file contains extra utilities for parsing and wrapping.
 */

#ifndef svtkParseExtras_h
#define svtkParseExtras_h

#include "svtkParseData.h"
#include "svtkWrappingToolsModule.h"
#include <stddef.h>

/* Flags for selecting what info to print for declarations */
#define SVTK_PARSE_NAMES 0x00000010
#define SVTK_PARSE_VALUES 0x00000020
#define SVTK_PARSE_RETURN_VALUE 0x00000040
#define SVTK_PARSE_PARAMETER_LIST 0x00000080
#define SVTK_PARSE_SPECIFIERS 0x00FF0000
#define SVTK_PARSE_TRAILERS 0x0F000000
#define SVTK_PARSE_TEMPLATES 0xF0000000
#define SVTK_PARSE_EVERYTHING 0xFFFFFFFF

#ifdef __cplusplus
extern "C"
{
#endif

  /**
   * Skip over a sequence of characters that begin with an alphabetic
   * character or an underscore, and include only alphanumeric
   * characters or underscores. Return the number of characters.
   */
  SVTKWRAPPINGTOOLS_EXPORT
  size_t svtkParse_IdentifierLength(const char* text);

  /**
   * Skip over a name, including any namespace prefixes and
   * any template arguments.  Return the number of characters.
   * Examples are "name", "::name", "name<arg>", "name::name2",
   * "::name::name2<arg1,arg2>".
   */
  SVTKWRAPPINGTOOLS_EXPORT
  size_t svtkParse_NameLength(const char* text);

  /**
   * Skip over a name, including any template arguments, but stopping
   * if a '::' is encountered.  Return the number of characters.
   * Examples are "name" and "name<arg>"
   */
  SVTKWRAPPINGTOOLS_EXPORT
  size_t svtkParse_UnscopedNameLength(const char* text);

  /**
   * Skip over a literal, which may be a number, a char in single
   * quotes, a string in double quotes, or a name, or a name followed
   * by arguments in parentheses.
   */
  SVTKWRAPPINGTOOLS_EXPORT
  size_t svtkParse_LiteralLength(const char* text);

  /**
   * Get a type from a type name, and return the number of characters used.
   * If the "classname" argument is not NULL, then it is used to return
   * the short name for the type, e.g. "long int" becomes "long", while
   * typedef names and class names are returned unchanged.  If "const"
   * appears in the type name, then the const bit flag is set for the
   * type, but "const" will not appear in the returned classname.
   */
  SVTKWRAPPINGTOOLS_EXPORT
  size_t svtkParse_BasicTypeFromString(
    const char* text, unsigned int* type, const char** classname, size_t* classname_len);

  /**
   * Generate a ValueInfo by parsing the type from the provided text.
   * Only simple text strings are supported, e.g. "const T **".
   * Returns the number of characters consumed.
   */
  SVTKWRAPPINGTOOLS_EXPORT
  size_t svtkParse_ValueInfoFromString(ValueInfo* val, StringCache* cache, const char* text);

  /**
   * Generate a C++ declaration string from a ValueInfo struct.
   *
   * The resulting string can represent a parameter declaration, a variable
   * or type declaration, or a function return value.  To use this function,
   * you should call it twice: first, call it with "text" set to NULL, and
   * it will return a lower bound on the length of the string that will be
   * produced.  Next, allocate at least length+1 bytes for the string, and
   * and call it again with "text" set to the allocated string.  The number
   * of bytes written to the string (not including the terminating null) will
   * be returned.
   *
   * The flags provide a mask that controls what information will be included,
   * for example SVTK_PARSE_CONST|SVTK_PARSE_VOLATILE is needed to include the
   * 'const' or 'volatile' qualifiers.  Use SVTK_PARSE_EVERYTHING to generate
   * the entire declaration for a variable or parameter.
   */
  SVTKWRAPPINGTOOLS_EXPORT
  size_t svtkParse_ValueInfoToString(ValueInfo* data, char* text, unsigned int flags);

  /**
   * Generate a C++ function signature from a FunctionInfo struct.
   *
   * See svtkParse_ValueInfoToString() for basic usage.  The flags can be set
   * to SVTK_PARSE_RETURN_VALUE to print only the return value for the function,
   * or SVTK_PARSE_PARAMETER_LIST to print only the parameter list.
   */
  SVTKWRAPPINGTOOLS_EXPORT
  size_t svtkParse_FunctionInfoToString(FunctionInfo* func, char* text, unsigned int flags);

  /**
   * Generate a C++ template declaration from a TemplateInfo struct.
   *
   * See svtkParse_ValueInfoToString() for basic usage.
   */
  SVTKWRAPPINGTOOLS_EXPORT
  size_t svtkParse_TemplateInfoToString(TemplateInfo* func, char* text, unsigned int flags);

  /**
   * Compare two C++ functions to see if they have the same signature.
   *
   * The following are the possible return values.  Any non-zero return value
   * means that the parameters match.  If the 2nd bit is also set, then the
   * function return value also matches.  If the 3rd bit is set, then the
   * parameters match and both methods are members of the same class, and
   * the constness of the functions match.  This means that the signatures
   * are not identical unless the return value is 7 or higher (.
   */
  SVTKWRAPPINGTOOLS_EXPORT
  int svtkParse_CompareFunctionSignature(const FunctionInfo* func1, const FunctionInfo* func2);

  /**
   * Expand a typedef within a variable, parameter, or typedef declaration.
   * The expansion is done in-place.
   */
  SVTKWRAPPINGTOOLS_EXPORT
  void svtkParse_ExpandTypedef(ValueInfo* valinfo, ValueInfo* typedefinfo);

  /**
   * Expand any unrecognized types within a variable, parameter, or typedef
   * that match any of the supplied typedefs. The expansion is done in-place.
   */
  SVTKWRAPPINGTOOLS_EXPORT
  void svtkParse_ExpandTypedefs(ValueInfo* valinfo, StringCache* cache, int n, const char* name[],
    const char* val[], ValueInfo* typedefinfo[]);

  /**
   * Wherever one of the specified names exists inside a Value or inside
   * a Dimension size, replace it with the corresponding val string.
   * This is used to replace constants with their values.
   */
  SVTKWRAPPINGTOOLS_EXPORT
  void svtkParse_ExpandValues(
    ValueInfo* valinfo, StringCache* cache, int n, const char* name[], const char* val[]);

  /**
   * Search and replace, return the initial string if no replacements
   * occurred, else return a new string allocated with malloc. */
  SVTKWRAPPINGTOOLS_EXPORT
  const char* svtkParse_StringReplace(
    const char* str1, int n, const char* name[], const char* val[]);

  /**
   * Extract the class name and template args from a templated
   * class type ID.  Returns the full number of characters that
   * were consumed during the decomposition.
   */
  SVTKWRAPPINGTOOLS_EXPORT
  size_t svtkParse_DecomposeTemplatedType(
    const char* text, const char** classname, int n, const char*** args, const char* defaults[]);

  /**
   * Free the list of strings returned by ExtractTemplateArgs.
   */
  SVTKWRAPPINGTOOLS_EXPORT
  void svtkParse_FreeTemplateDecomposition(const char* classname, int n, const char** args);

  /**
   * Instantiate a class template by substituting the provided arguments
   * for the template parameters. If "n" is less than the number of template
   * parameters, then default parameter values (if present) will be used.
   * If an error occurs, the error will be printed to stderr and NULL will
   * be returned.
   */
  SVTKWRAPPINGTOOLS_EXPORT
  void svtkParse_InstantiateClassTemplate(
    ClassInfo* data, StringCache* cache, int n, const char* args[]);

  /**
   * Instantiate a function or class method template by substituting the
   * provided arguments for the template parameters.  If "n" is less than
   * the number of template parameters, then default parameter values
   * (if present) will be used.  If an error occurs, the error will be
   * printed to stderr and NULL will be returned.
   */
  SVTKWRAPPINGTOOLS_EXPORT
  void svtkParse_IntantiateFunctionTemplate(FunctionInfo* data, int n, const char* args[]);

  /**
   * Get a zero-terminated array of the types in svtkTemplateMacro.
   */
  SVTKWRAPPINGTOOLS_EXPORT
  const char** svtkParse_GetTemplateMacroTypes(void);

  /**
   * Get a zero-terminated array of the types in svtkArray.
   */
  SVTKWRAPPINGTOOLS_EXPORT
  const char** svtkParse_GetArrayTypes(void);

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif
/* SVTK-HeaderTest-Exclude: svtkParseExtras.h */
