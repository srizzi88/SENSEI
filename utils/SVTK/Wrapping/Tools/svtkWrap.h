/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkWrap.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

/**
 * svtkWrap provides useful functions for generating wrapping code.
 */

#ifndef svtkWrap_h
#define svtkWrap_h

#include "svtkParse.h"
#include "svtkParseHierarchy.h"
#include "svtkWrappingToolsModule.h"

/**
 * For use with svtkWrap_DeclareVariable.
 */
/*@{*/
#define SVTK_WRAP_RETURN 1
#define SVTK_WRAP_ARG 2
#define SVTK_WRAP_NOSEMI 4
/*@}*/

#ifdef __cplusplus
extern "C"
{
#endif

  /**
   * Check for common types.
   * IsPODPointer is for unsized arrays of POD types.
   * IsZeroCopyPointer is for buffers that shouldn't be copied.
   */
  /*@{*/
  SVTKWRAPPINGTOOLS_EXPORT int svtkWrap_IsVoid(ValueInfo* val);
  SVTKWRAPPINGTOOLS_EXPORT int svtkWrap_IsVoidFunction(ValueInfo* val);
  SVTKWRAPPINGTOOLS_EXPORT int svtkWrap_IsVoidPointer(ValueInfo* val);
  SVTKWRAPPINGTOOLS_EXPORT int svtkWrap_IsCharPointer(ValueInfo* val);
  SVTKWRAPPINGTOOLS_EXPORT int svtkWrap_IsPODPointer(ValueInfo* val);
  SVTKWRAPPINGTOOLS_EXPORT int svtkWrap_IsZeroCopyPointer(ValueInfo* val);
  SVTKWRAPPINGTOOLS_EXPORT int svtkWrap_IsStdVector(ValueInfo* val);
  SVTKWRAPPINGTOOLS_EXPORT int svtkWrap_IsSVTKObject(ValueInfo* val);
  SVTKWRAPPINGTOOLS_EXPORT int svtkWrap_IsSpecialObject(ValueInfo* val);
  SVTKWRAPPINGTOOLS_EXPORT int svtkWrap_IsPythonObject(ValueInfo* val);
  /*@}*/

  /**
   * The basic types, all are mutually exclusive.
   * Note that enums are considered to be objects,
   * bool and char are considered to be numeric.
   */
  /*@{*/
  SVTKWRAPPINGTOOLS_EXPORT int svtkWrap_IsObject(ValueInfo* val);
  SVTKWRAPPINGTOOLS_EXPORT int svtkWrap_IsFunction(ValueInfo* val);
  SVTKWRAPPINGTOOLS_EXPORT int svtkWrap_IsStream(ValueInfo* val);
  SVTKWRAPPINGTOOLS_EXPORT int svtkWrap_IsNumeric(ValueInfo* val);
  SVTKWRAPPINGTOOLS_EXPORT int svtkWrap_IsString(ValueInfo* val);
  /*@}*/

  /**
   * Subcategories of numeric types.  In this categorization,
   * bool and char are not considered to be integers.
   */
  /*@{*/
  SVTKWRAPPINGTOOLS_EXPORT int svtkWrap_IsBool(ValueInfo* val);
  SVTKWRAPPINGTOOLS_EXPORT int svtkWrap_IsChar(ValueInfo* val);
  SVTKWRAPPINGTOOLS_EXPORT int svtkWrap_IsInteger(ValueInfo* val);
  SVTKWRAPPINGTOOLS_EXPORT int svtkWrap_IsRealNumber(ValueInfo* val);
  /*@}*/

  /**
   * Arrays and pointers. These are mutually exclusive.
   * IsPointer() does not include pointers to pointers.
   * IsArray() and IsNArray() do not include unsized arrays.
   * Arrays of pointers are not included in any of these.
   */
  /*@{*/
  SVTKWRAPPINGTOOLS_EXPORT int svtkWrap_IsScalar(ValueInfo* val);
  SVTKWRAPPINGTOOLS_EXPORT int svtkWrap_IsPointer(ValueInfo* val);
  SVTKWRAPPINGTOOLS_EXPORT int svtkWrap_IsArray(ValueInfo* val);
  SVTKWRAPPINGTOOLS_EXPORT int svtkWrap_IsNArray(ValueInfo* val);
  /*@}*/

  /**
   * Properties that can combine with other properties.
   */
  /*@{*/
  SVTKWRAPPINGTOOLS_EXPORT int svtkWrap_IsNonConstRef(ValueInfo* val);
  SVTKWRAPPINGTOOLS_EXPORT int svtkWrap_IsConstRef(ValueInfo* val);
  SVTKWRAPPINGTOOLS_EXPORT int svtkWrap_IsRef(ValueInfo* val);
  SVTKWRAPPINGTOOLS_EXPORT int svtkWrap_IsConst(ValueInfo* val);
  /*@}*/

  /**
   * Hints.
   * NewInstance objects must be freed by the caller.
   */
  /*@{*/
  SVTKWRAPPINGTOOLS_EXPORT int svtkWrap_IsNewInstance(ValueInfo* val);
  /*@}*/

  /**
   * Check whether the class is derived from svtkObjectBase.
   * If "hinfo" is NULL, this just checks that the class
   * name starts with "svtk".
   */
  SVTKWRAPPINGTOOLS_EXPORT int svtkWrap_IsSVTKObjectBaseType(
    HierarchyInfo* hinfo, const char* classname);

  /**
   * Check whether the class is not derived from svtkObjectBase.
   * If "hinfo" is NULL, it defaults to just checking if
   * the class starts with "svtk" and returns -1 if so.
   */
  SVTKWRAPPINGTOOLS_EXPORT int svtkWrap_IsSpecialType(HierarchyInfo* hinfo, const char* classname);

  /**
   * Check if the class is derived from superclass.
   * If "hinfo" is NULL, then only an exact match to the
   * superclass will succeed.
   */
  SVTKWRAPPINGTOOLS_EXPORT int svtkWrap_IsTypeOf(
    HierarchyInfo* hinfo, const char* classname, const char* superclass);

  /**
   * Check if the type of the value is an enum member of the class.
   */
  SVTKWRAPPINGTOOLS_EXPORT int svtkWrap_IsEnumMember(ClassInfo* data, ValueInfo* arg);

  /**
   * Check whether a class is wrapped.  If "hinfo" is NULL,
   * it just checks that the class starts with "svtk".
   */
  SVTKWRAPPINGTOOLS_EXPORT int svtkWrap_IsClassWrapped(HierarchyInfo* hinfo, const char* classname);

  /**
   * Check whether the destructor is public
   */
  SVTKWRAPPINGTOOLS_EXPORT int svtkWrap_HasPublicDestructor(ClassInfo* data);

  /**
   * Check whether the copy constructor is public
   */
  SVTKWRAPPINGTOOLS_EXPORT int svtkWrap_HasPublicCopyConstructor(ClassInfo* data);

  /**
   * Expand all typedef types that are used in function arguments.
   * This should be done before any wrapping is done, to make sure
   * that the wrappers see the real types.
   */
  SVTKWRAPPINGTOOLS_EXPORT void svtkWrap_ExpandTypedefs(
    ClassInfo* data, FileInfo* finfo, HierarchyInfo* hinfo);

  /**
   * Apply any using declarations that appear in the class.
   * If any using declarations appear in the class that refer to superclass
   * methods, the superclass header file will be parsed and the used methods
   * will be brought into the class.
   */
  SVTKWRAPPINGTOOLS_EXPORT void svtkWrap_ApplyUsingDeclarations(
    ClassInfo* data, FileInfo* finfo, HierarchyInfo* hinfo);

  /**
   * Merge members of all superclasses into the data structure.
   * The superclass header files will be read and parsed.
   */
  SVTKWRAPPINGTOOLS_EXPORT void svtkWrap_MergeSuperClasses(
    ClassInfo* data, FileInfo* finfo, HierarchyInfo* hinfo);

  /**
   * Apply any hints about array sizes, e.g. hint that the
   * GetNumberOfComponents() method gives the tuple size.
   */
  SVTKWRAPPINGTOOLS_EXPORT void svtkWrap_FindCountHints(
    ClassInfo* data, FileInfo* finfo, HierarchyInfo* hinfo);

  /**
   * Get the size of a fixed-size tuple
   */
  SVTKWRAPPINGTOOLS_EXPORT int svtkWrap_GetTupleSize(ClassInfo* data, HierarchyInfo* hinfo);

  /**
   * Apply any hints about methods that return a new object instance,
   * i.e. factory methods and the like.  Reference counts must be
   * handled differently for such returned objects.
   */
  SVTKWRAPPINGTOOLS_EXPORT void svtkWrap_FindNewInstanceMethods(
    ClassInfo* data, HierarchyInfo* hinfo);

  /**
   * Get the name of a type.  The name will not include "const".
   */
  SVTKWRAPPINGTOOLS_EXPORT const char* svtkWrap_GetTypeName(ValueInfo* val);

  /**
   * True if the method a constructor of the class.
   */
  SVTKWRAPPINGTOOLS_EXPORT int svtkWrap_IsConstructor(ClassInfo* c, FunctionInfo* f);

  /**
   * True if the method a destructor of the class.
   */
  SVTKWRAPPINGTOOLS_EXPORT int svtkWrap_IsDestructor(ClassInfo* c, FunctionInfo* f);

  /**
   * True if the method is inherited from a base class.
   */
  SVTKWRAPPINGTOOLS_EXPORT int svtkWrap_IsInheritedMethod(ClassInfo* c, FunctionInfo* f);

  /**
   * Check if a method is from a SetVector method.
   */
  SVTKWRAPPINGTOOLS_EXPORT int svtkWrap_IsSetVectorMethod(FunctionInfo* f);

  /**
   * Check if a method is from a GetVector method.
   */
  SVTKWRAPPINGTOOLS_EXPORT int svtkWrap_IsGetVectorMethod(FunctionInfo* f);

  /**
   * Count the number of parameters that are wrapped.
   * This skips the "void *" parameter that follows
   * wrapped function pointer parameters.
   */
  SVTKWRAPPINGTOOLS_EXPORT int svtkWrap_CountWrappedParameters(FunctionInfo* f);

  /**
   * Count the number of args that are required.
   * This counts to the last argument that does not
   * have a default value.  Array args are not allowed
   * to have default values.
   */
  SVTKWRAPPINGTOOLS_EXPORT int svtkWrap_CountRequiredArguments(FunctionInfo* f);

  /**
   * Write a variable declaration to a file.
   * Void is automatically ignored, and nothing is written for
   * function pointers
   * Set "idx" to -1 to avoid writing an idx.
   * Set "flags" to SVTK_WRAP_RETURN to write a return value,
   * or to SVTK_WRAP_ARG to write a temp argument variable.
   * The following rules apply:
   * - if SVTK_WRAP_NOSEMI is set, then no semicolon/newline is printed
   * - if SVTK_WRAP_RETURN is set, then "&" becomes "*"
   * - if SVTK_WRAP_ARG is set, "&" becomes "*" only for object
   *   types, and is removed for all other types.
   * - "const" is removed except for return values with "&" or "*".
   */
  SVTKWRAPPINGTOOLS_EXPORT void svtkWrap_DeclareVariable(
    FILE* fp, ClassInfo* data, ValueInfo* v, const char* name, int idx, int flags);

  /**
   * Write an "int" size variable for arrays, initialized to
   * the array size if the size is greater than zero.
   * For N-dimensional arrays, write a static array of ints.
   */
  SVTKWRAPPINGTOOLS_EXPORT void svtkWrap_DeclareVariableSize(
    FILE* fp, ValueInfo* v, const char* name, int idx);

  /**
   * Qualify all the unqualified identifiers in the given expression
   * and print the result to the file.
   */
  SVTKWRAPPINGTOOLS_EXPORT void svtkWrap_QualifyExpression(
    FILE* fp, ClassInfo* data, const char* text);

  /**
   * Makes a superclass name into a valid identifier. Returns NULL if the given
   * name is valid as-is.
   */
  SVTKWRAPPINGTOOLS_EXPORT char* svtkWrap_SafeSuperclassName(const char* name);

#ifdef __cplusplus
}
#endif

#endif
/* SVTK-HeaderTest-Exclude: svtkWrap.h */
