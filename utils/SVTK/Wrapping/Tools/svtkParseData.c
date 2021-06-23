/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkParseData.c

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/*-------------------------------------------------------------------------
  Copyright (c) 2010 David Gobbi.

  Contributed to the VisualizationToolkit by the author in May 2010
  under the terms of the Visualization Toolkit 2008 copyright.
-------------------------------------------------------------------------*/

#include "svtkParseData.h"
#include <stdlib.h>
#include <string.h>

/* Initialize the FileInfo struct */
void svtkParse_InitFile(FileInfo* file_info)
{
  /* file info */
  file_info->FileName = NULL;
  file_info->NameComment = NULL;
  file_info->Description = NULL;
  file_info->Caveats = NULL;
  file_info->SeeAlso = NULL;

  file_info->NumberOfIncludes = 0;
  file_info->Includes = NULL;
  file_info->MainClass = NULL;
  file_info->Contents = NULL;

  file_info->Strings = NULL;
}

/* Free the FileInfo struct */
void svtkParse_FreeFile(FileInfo* file_info)
{
  int i, n;

  n = file_info->NumberOfIncludes;
  for (i = 0; i < n; i++)
  {
    svtkParse_FreeFile(file_info->Includes[i]);
    free(file_info->Includes[i]);
  }
  if (file_info->Includes)
  {
    free(file_info->Includes);
  }

  svtkParse_FreeNamespace(file_info->Contents);
  file_info->Contents = NULL;
}

/* Initialize a CommentInfo struct */
void svtkParse_InitComment(CommentInfo* info)
{
  info->Type = DOX_COMMAND_OTHER;
  info->Comment = NULL;
  info->Name = NULL;
}

/* Copy a CommentInfo struct */
void svtkParse_CopyComment(CommentInfo* info, const CommentInfo* orig)
{
  info->Type = orig->Type;
  info->Comment = orig->Comment;
  info->Name = orig->Name;
}

/* Free a CommentInfo struct */
void svtkParse_FreeComment(CommentInfo* info)
{
  free(info);
}

/* Initialize a TemplateInfo struct */
void svtkParse_InitTemplate(TemplateInfo* info)
{
  info->NumberOfParameters = 0;
  info->Parameters = NULL;
}

/* Copy a TemplateInfo struct */
void svtkParse_CopyTemplate(TemplateInfo* info, const TemplateInfo* orig)
{
  int i, n;

  n = orig->NumberOfParameters;
  info->NumberOfParameters = n;
  info->Parameters = (ValueInfo**)malloc(n * sizeof(ValueInfo*));

  for (i = 0; i < n; i++)
  {
    info->Parameters[i] = (ValueInfo*)malloc(sizeof(ValueInfo));
    svtkParse_CopyValue(info->Parameters[i], orig->Parameters[i]);
  }
}

/* Free a TemplateInfo struct */
void svtkParse_FreeTemplate(TemplateInfo* template_info)
{
  int j, m;

  m = template_info->NumberOfParameters;
  for (j = 0; j < m; j++)
  {
    svtkParse_FreeValue(template_info->Parameters[j]);
  }

  if (m > 0)
  {
    free(template_info->Parameters);
  }

  free(template_info);
}

/* Initialize a Function struct */
void svtkParse_InitFunction(FunctionInfo* func)
{
#ifndef SVTK_PARSE_LEGACY_REMOVE
  int i;
#endif

  func->ItemType = SVTK_FUNCTION_INFO;
  func->Access = SVTK_ACCESS_PUBLIC;
  func->Name = NULL;
  func->Comment = NULL;
  func->Class = NULL;
  func->Signature = NULL;
  func->Template = NULL;
  func->NumberOfParameters = 0;
  func->Parameters = NULL;
  func->ReturnValue = NULL;
  func->NumberOfPreconds = 0;
  func->Preconds = NULL;
  func->Macro = NULL;
  func->SizeHint = NULL;
  func->IsStatic = 0;
  func->IsVirtual = 0;
  func->IsPureVirtual = 0;
  func->IsOperator = 0;
  func->IsVariadic = 0;
  func->IsConst = 0;
  func->IsDeleted = 0;
  func->IsFinal = 0;
  func->IsOverride = 0;
  func->IsExplicit = 0;
  func->IsLegacy = 0;
  func->IsExcluded = 0;

#ifndef SVTK_PARSE_LEGACY_REMOVE
  /* everything below here is legacy information, *
   * maintained only for backwards compatibility  */
  func->NumberOfArguments = 0;
  func->ReturnType = SVTK_PARSE_VOID;
  func->ReturnClass = NULL;
  func->HaveHint = 0;
  func->HintSize = 0;
  func->ArrayFailure = 0;
  func->IsPublic = 0;
  func->IsProtected = 0;

  for (i = 0; i < MAX_ARGS; i++)
  {
    func->ArgTypes[i] = 0;
    func->ArgClasses[i] = 0;
    func->ArgCounts[i] = 0;
  }
#endif
}

/* Copy a Function struct */
void svtkParse_CopyFunction(FunctionInfo* func, const FunctionInfo* orig)
{
  int i, n;

  func->ItemType = orig->ItemType;
  func->Access = orig->Access;
  func->Name = orig->Name;
  func->Comment = orig->Comment;
  func->Class = orig->Class;
  func->Signature = orig->Signature;
  func->Template = NULL;

  if (orig->Template)
  {
    func->Template = (TemplateInfo*)malloc(sizeof(TemplateInfo));
    svtkParse_CopyTemplate(func->Template, orig->Template);
  }

  n = orig->NumberOfParameters;
  func->NumberOfParameters = n;
  if (n)
  {
    func->Parameters = (ValueInfo**)malloc(n * sizeof(ValueInfo*));
    for (i = 0; i < n; i++)
    {
      func->Parameters[i] = (ValueInfo*)malloc(sizeof(ValueInfo));
      svtkParse_CopyValue(func->Parameters[i], orig->Parameters[i]);
    }
  }

  func->ReturnValue = NULL;
  if (orig->ReturnValue)
  {
    func->ReturnValue = (ValueInfo*)malloc(sizeof(ValueInfo));
    svtkParse_CopyValue(func->ReturnValue, orig->ReturnValue);
  }

  n = orig->NumberOfPreconds;
  func->NumberOfPreconds = n;
  if (n)
  {
    func->Preconds = (const char**)malloc(n * sizeof(const char*));
    for (i = 0; i < n; i++)
    {
      func->Preconds[i] = orig->Preconds[i];
    }
  }

  func->Macro = orig->Macro;
  func->SizeHint = orig->SizeHint;
  func->IsStatic = orig->IsStatic;
  func->IsVirtual = orig->IsVirtual;
  func->IsPureVirtual = orig->IsPureVirtual;
  func->IsOperator = orig->IsOperator;
  func->IsVariadic = orig->IsVariadic;
  func->IsConst = orig->IsConst;
  func->IsDeleted = orig->IsDeleted;
  func->IsFinal = orig->IsFinal;
  func->IsOverride = orig->IsOverride;
  func->IsExplicit = orig->IsExplicit;
  func->IsLegacy = orig->IsLegacy;
  func->IsExcluded = orig->IsExcluded;

#ifndef SVTK_PARSE_LEGACY_REMOVE
  /* everything below here is legacy information, *
   * maintained only for backwards compatibility  */
  func->NumberOfArguments = orig->NumberOfArguments;
  func->ReturnType = orig->ReturnType;
  func->ReturnClass = orig->ReturnClass;
  func->HaveHint = orig->HaveHint;
  func->HintSize = orig->HintSize;
  func->ArrayFailure = orig->ArrayFailure;
  func->IsPublic = orig->IsPublic;
  func->IsProtected = orig->IsProtected;

  for (i = 0; i < MAX_ARGS; i++)
  {
    func->ArgTypes[i] = orig->ArgTypes[i];
    func->ArgClasses[i] = orig->ArgClasses[i];
    func->ArgCounts[i] = orig->ArgCounts[i];
  }
#endif
}

/* Free a Function struct */
void svtkParse_FreeFunction(FunctionInfo* function_info)
{
  int j, m;

  if (function_info->Template)
  {
    svtkParse_FreeTemplate(function_info->Template);
  }

  m = function_info->NumberOfParameters;
  for (j = 0; j < m; j++)
  {
    svtkParse_FreeValue(function_info->Parameters[j]);
  }
  if (m > 0)
  {
    free(function_info->Parameters);
  }

  if (function_info->ReturnValue)
  {
    svtkParse_FreeValue(function_info->ReturnValue);
  }

  if (function_info->NumberOfPreconds > 0)
  {
    free((char**)function_info->Preconds);
  }

  free(function_info);
}

/* Initialize a Value struct */
void svtkParse_InitValue(ValueInfo* val)
{
  val->ItemType = SVTK_VARIABLE_INFO;
  val->Access = SVTK_ACCESS_PUBLIC;
  val->Name = NULL;
  val->Comment = NULL;
  val->Value = NULL;
  val->Type = 0;
  val->Class = NULL;
  val->Count = 0;
  val->CountHint = NULL;
  val->NumberOfDimensions = 0;
  val->Dimensions = NULL;
  val->Function = NULL;
  val->Template = NULL;
  val->IsStatic = 0;
  val->IsEnum = 0;
  val->IsPack = 0;
}

/* Copy a Value struct */
void svtkParse_CopyValue(ValueInfo* val, const ValueInfo* orig)
{
  int i, n;

  val->ItemType = orig->ItemType;
  val->Access = orig->Access;
  val->Name = orig->Name;
  val->Comment = orig->Comment;
  val->Value = orig->Value;
  val->Type = orig->Type;
  val->Class = orig->Class;
  val->Count = orig->Count;
  val->CountHint = orig->CountHint;

  n = orig->NumberOfDimensions;
  val->NumberOfDimensions = n;
  if (n)
  {
    val->Dimensions = (const char**)malloc(n * sizeof(char*));
    for (i = 0; i < n; i++)
    {
      val->Dimensions[i] = orig->Dimensions[i];
    }
  }

  val->Function = NULL;
  if (orig->Function)
  {
    val->Function = (FunctionInfo*)malloc(sizeof(FunctionInfo));
    svtkParse_CopyFunction(val->Function, orig->Function);
  }

  val->Template = NULL;
  if (orig->Template)
  {
    val->Template = (TemplateInfo*)malloc(sizeof(TemplateInfo));
    svtkParse_CopyTemplate(val->Template, orig->Template);
  }

  val->IsStatic = orig->IsStatic;
  val->IsEnum = orig->IsEnum;
  val->IsPack = orig->IsPack;
}

/* Free a Value struct */
void svtkParse_FreeValue(ValueInfo* value_info)
{
  if (value_info->NumberOfDimensions)
  {
    free((char**)value_info->Dimensions);
  }
  if (value_info->Function)
  {
    svtkParse_FreeFunction(value_info->Function);
  }
  if (value_info->Template)
  {
    svtkParse_FreeTemplate(value_info->Template);
  }

  free(value_info);
}

/* Initialize an Enum struct */
void svtkParse_InitEnum(EnumInfo* item)
{
  svtkParse_InitClass(item);
  item->ItemType = SVTK_ENUM_INFO;
}

/* Copy an Enum struct */
void svtkParse_CopyEnum(EnumInfo* item, const EnumInfo* orig)
{
  svtkParse_CopyClass(item, orig);
}

/* Free an Enum struct */
void svtkParse_FreeEnum(EnumInfo* enum_info)
{
  free(enum_info);
}

/* Initialize a Using struct */
void svtkParse_InitUsing(UsingInfo* item)
{
  item->ItemType = SVTK_USING_INFO;
  item->Access = SVTK_ACCESS_PUBLIC;
  item->Name = NULL;
  item->Comment = NULL;
  item->Scope = NULL;
}

/* Copy a Using struct */
void svtkParse_CopyUsing(UsingInfo* item, const UsingInfo* orig)
{
  item->ItemType = orig->ItemType;
  item->Access = orig->Access;
  item->Name = orig->Name;
  item->Comment = orig->Comment;
  item->Scope = orig->Scope;
}

/* Free a Using struct */
void svtkParse_FreeUsing(UsingInfo* using_info)
{
  free(using_info);
}

/* Initialize a Class struct */
void svtkParse_InitClass(ClassInfo* cls)
{
  cls->ItemType = SVTK_CLASS_INFO;
  cls->Access = SVTK_ACCESS_PUBLIC;
  cls->Name = NULL;
  cls->Comment = NULL;
  cls->Template = NULL;
  cls->NumberOfSuperClasses = 0;
  cls->SuperClasses = NULL;
  cls->NumberOfItems = 0;
  cls->Items = NULL;
  cls->NumberOfClasses = 0;
  cls->Classes = NULL;
  cls->NumberOfFunctions = 0;
  cls->Functions = NULL;
  cls->NumberOfConstants = 0;
  cls->Constants = NULL;
  cls->NumberOfVariables = 0;
  cls->Variables = NULL;
  cls->NumberOfEnums = 0;
  cls->Enums = NULL;
  cls->NumberOfTypedefs = 0;
  cls->Typedefs = NULL;
  cls->NumberOfUsings = 0;
  cls->Usings = NULL;
  cls->NumberOfNamespaces = 0;
  cls->Namespaces = NULL;
  cls->NumberOfComments = 0;
  cls->Comments = NULL;
  cls->IsAbstract = 0;
  cls->IsFinal = 0;
  cls->HasDelete = 0;
  cls->IsExcluded = 0;
}

/* Copy a Class struct */
void svtkParse_CopyClass(ClassInfo* cls, const ClassInfo* orig)
{
  int i, n;

  cls->ItemType = orig->ItemType;
  cls->Access = orig->Access;
  cls->Name = orig->Name;
  cls->Comment = orig->Comment;
  cls->Template = NULL;

  if (orig->Template)
  {
    cls->Template = (TemplateInfo*)malloc(sizeof(TemplateInfo));
    svtkParse_CopyTemplate(cls->Template, orig->Template);
  }

  n = orig->NumberOfSuperClasses;
  cls->NumberOfSuperClasses = n;
  if (n)
  {
    cls->SuperClasses = (const char**)malloc(n * sizeof(char*));
    for (i = 0; i < n; i++)
    {
      cls->SuperClasses[i] = orig->SuperClasses[i];
    }
  }

  n = orig->NumberOfItems;
  cls->NumberOfItems = n;
  if (n)
  {
    cls->Items = (ItemInfo*)malloc(n * sizeof(ItemInfo));
    for (i = 0; i < n; i++)
    {
      cls->Items[i].Type = orig->Items[i].Type;
      cls->Items[i].Index = orig->Items[i].Index;
    }
  }

  n = orig->NumberOfClasses;
  cls->NumberOfClasses = n;
  if (n)
  {
    cls->Classes = (ClassInfo**)malloc(n * sizeof(ClassInfo*));
    for (i = 0; i < n; i++)
    {
      cls->Classes[i] = (ClassInfo*)malloc(sizeof(ClassInfo));
      svtkParse_CopyClass(cls->Classes[i], orig->Classes[i]);
    }
  }

  n = orig->NumberOfFunctions;
  cls->NumberOfFunctions = n;
  if (n)
  {
    cls->Functions = (FunctionInfo**)malloc(n * sizeof(FunctionInfo*));
    for (i = 0; i < n; i++)
    {
      cls->Functions[i] = (FunctionInfo*)malloc(sizeof(FunctionInfo));
      svtkParse_CopyFunction(cls->Functions[i], orig->Functions[i]);
    }
  }

  n = orig->NumberOfConstants;
  cls->NumberOfConstants = n;
  if (n)
  {
    cls->Constants = (ValueInfo**)malloc(n * sizeof(ValueInfo*));
    for (i = 0; i < n; i++)
    {
      cls->Constants[i] = (ValueInfo*)malloc(sizeof(ValueInfo));
      svtkParse_CopyValue(cls->Constants[i], orig->Constants[i]);
    }
  }

  n = orig->NumberOfVariables;
  cls->NumberOfVariables = n;
  if (n)
  {
    cls->Variables = (ValueInfo**)malloc(n * sizeof(ValueInfo*));
    for (i = 0; i < n; i++)
    {
      cls->Variables[i] = (ValueInfo*)malloc(sizeof(ValueInfo));
      svtkParse_CopyValue(cls->Variables[i], orig->Variables[i]);
    }
  }

  n = orig->NumberOfEnums;
  cls->NumberOfEnums = n;
  if (n)
  {
    cls->Enums = (EnumInfo**)malloc(n * sizeof(EnumInfo*));
    for (i = 0; i < n; i++)
    {
      cls->Enums[i] = (EnumInfo*)malloc(sizeof(EnumInfo));
      svtkParse_CopyEnum(cls->Enums[i], orig->Enums[i]);
    }
  }

  n = orig->NumberOfTypedefs;
  cls->NumberOfTypedefs = n;
  if (n)
  {
    cls->Typedefs = (ValueInfo**)malloc(n * sizeof(ValueInfo*));
    for (i = 0; i < n; i++)
    {
      cls->Typedefs[i] = (ValueInfo*)malloc(sizeof(ValueInfo));
      svtkParse_CopyValue(cls->Typedefs[i], orig->Typedefs[i]);
    }
  }

  n = orig->NumberOfUsings;
  cls->NumberOfUsings = n;
  if (n)
  {
    cls->Usings = (UsingInfo**)malloc(n * sizeof(UsingInfo*));
    for (i = 0; i < n; i++)
    {
      cls->Usings[i] = (UsingInfo*)malloc(sizeof(UsingInfo));
      svtkParse_CopyUsing(cls->Usings[i], orig->Usings[i]);
    }
  }

  n = orig->NumberOfNamespaces;
  cls->NumberOfNamespaces = n;
  if (n)
  {
    cls->Namespaces = (NamespaceInfo**)malloc(n * sizeof(NamespaceInfo*));
    for (i = 0; i < n; i++)
    {
      cls->Namespaces[i] = (NamespaceInfo*)malloc(sizeof(NamespaceInfo));
      svtkParse_CopyNamespace(cls->Namespaces[i], orig->Namespaces[i]);
    }
  }

  n = orig->NumberOfComments;
  cls->NumberOfComments = n;
  if (n)
  {
    cls->Comments = (CommentInfo**)malloc(n * sizeof(CommentInfo*));
    for (i = 0; i < n; i++)
    {
      cls->Comments[i] = (CommentInfo*)malloc(sizeof(CommentInfo));
      svtkParse_CopyComment(cls->Comments[i], orig->Comments[i]);
    }
  }

  cls->IsAbstract = orig->IsAbstract;
  cls->IsFinal = orig->IsFinal;
  cls->HasDelete = orig->HasDelete;
  cls->IsExcluded = orig->IsExcluded;
}

/* Free a Class struct */
void svtkParse_FreeClass(ClassInfo* class_info)
{
  int j, m;

  if (class_info->Template)
  {
    svtkParse_FreeTemplate(class_info->Template);
  }

  m = class_info->NumberOfSuperClasses;
  if (m > 0)
  {
    free((char**)class_info->SuperClasses);
  }

  m = class_info->NumberOfClasses;
  for (j = 0; j < m; j++)
  {
    svtkParse_FreeClass(class_info->Classes[j]);
  }
  if (m > 0)
  {
    free(class_info->Classes);
  }

  m = class_info->NumberOfFunctions;
  for (j = 0; j < m; j++)
  {
    svtkParse_FreeFunction(class_info->Functions[j]);
  }
  if (m > 0)
  {
    free(class_info->Functions);
  }

  m = class_info->NumberOfConstants;
  for (j = 0; j < m; j++)
  {
    svtkParse_FreeValue(class_info->Constants[j]);
  }
  if (m > 0)
  {
    free(class_info->Constants);
  }

  m = class_info->NumberOfVariables;
  for (j = 0; j < m; j++)
  {
    svtkParse_FreeValue(class_info->Variables[j]);
  }
  if (m > 0)
  {
    free(class_info->Variables);
  }

  m = class_info->NumberOfEnums;
  for (j = 0; j < m; j++)
  {
    svtkParse_FreeEnum(class_info->Enums[j]);
  }
  if (m > 0)
  {
    free(class_info->Enums);
  }

  m = class_info->NumberOfTypedefs;
  for (j = 0; j < m; j++)
  {
    svtkParse_FreeValue(class_info->Typedefs[j]);
  }
  if (m > 0)
  {
    free(class_info->Typedefs);
  }

  m = class_info->NumberOfUsings;
  for (j = 0; j < m; j++)
  {
    svtkParse_FreeUsing(class_info->Usings[j]);
  }
  if (m > 0)
  {
    free(class_info->Usings);
  }

  m = class_info->NumberOfNamespaces;
  for (j = 0; j < m; j++)
  {
    svtkParse_FreeNamespace(class_info->Namespaces[j]);
  }
  if (m > 0)
  {
    free(class_info->Namespaces);
  }

  if (class_info->NumberOfItems > 0)
  {
    free(class_info->Items);
  }

  m = class_info->NumberOfComments;
  for (j = 0; j < m; j++)
  {
    svtkParse_FreeComment(class_info->Comments[j]);
  }
  if (m > 0)
  {
    free(class_info->Comments);
  }

  free(class_info);
}

/* Initialize a Namespace struct */
void svtkParse_InitNamespace(NamespaceInfo* name_info)
{
  svtkParse_InitClass(name_info);
  name_info->ItemType = SVTK_NAMESPACE_INFO;
}

/* Copy a Namespace struct */
void svtkParse_CopyNamespace(NamespaceInfo* ninfo, const NamespaceInfo* orig)
{
  svtkParse_CopyClass(ninfo, orig);
}

/* Free a Namespace struct */
void svtkParse_FreeNamespace(NamespaceInfo* namespace_info)
{
  svtkParse_FreeClass(namespace_info);
}

/* This method is used for extending dynamic arrays in a progression of
 * powers of two.  If "n" reaches a power of two, then the array size is
 * doubled so that "n" can be safely incremented. */
static void* array_size_check(void* arraymem, size_t size, int n)
{
  /* if empty, alloc for the first time */
  if (n == 0)
  {
    return malloc(size);
  }
  /* if count is power of two, reallocate with double size */
  else if ((n & (n - 1)) == 0)
  {
    return realloc(arraymem, (n << 1) * size);
  }

  /* no reallocation, just return the original array */
  return arraymem;
}

/* Utility method to add an included file to a FileInfo */
void svtkParse_AddIncludeToFile(FileInfo* file_info, FileInfo* include_file)
{
  file_info->Includes = (FileInfo**)array_size_check(
    (FileInfo**)file_info->Includes, sizeof(FileInfo*), file_info->NumberOfIncludes);

  file_info->Includes[file_info->NumberOfIncludes++] = include_file;

  if (!include_file->Strings)
  {
    include_file->Strings = file_info->Strings;
  }
}

/* Utility method to add a const char pointer to an array */
void svtkParse_AddStringToArray(const char*** valueArray, int* count, const char* value)
{
  *valueArray = (const char**)array_size_check((char**)*valueArray, sizeof(const char*), *count);

  (*valueArray)[(*count)++] = value;
}

/* Utility method to add an item to an array */
void svtkParse_AddItemToArray(ItemInfo** valueArray, int* count, parse_item_t type, int idx)
{
  int n = *count;
  ItemInfo* values = *valueArray;

  values = (ItemInfo*)array_size_check(values, sizeof(ItemInfo), n);

  values[n].Type = type;
  values[n].Index = idx;
  *count = n + 1;
  *valueArray = values;
}

/* Add a ClassInfo to a ClassInfo */
void svtkParse_AddClassToClass(ClassInfo* info, ClassInfo* item)
{
  svtkParse_AddItemToArray(
    &info->Items, &info->NumberOfItems, item->ItemType, info->NumberOfClasses);
  info->Classes =
    (ClassInfo**)array_size_check(info->Classes, sizeof(ClassInfo*), info->NumberOfClasses);
  info->Classes[info->NumberOfClasses++] = item;
}

/* Add a FunctionInfo to a ClassInfo */
void svtkParse_AddFunctionToClass(ClassInfo* info, FunctionInfo* item)
{
  svtkParse_AddItemToArray(
    &info->Items, &info->NumberOfItems, item->ItemType, info->NumberOfFunctions);
  info->Functions = (FunctionInfo**)array_size_check(
    info->Functions, sizeof(FunctionInfo*), info->NumberOfFunctions);
  info->Functions[info->NumberOfFunctions++] = item;
}

/* Add a EnumInfo to a ClassInfo */
void svtkParse_AddEnumToClass(ClassInfo* info, EnumInfo* item)
{
  svtkParse_AddItemToArray(&info->Items, &info->NumberOfItems, item->ItemType, info->NumberOfEnums);
  info->Enums = (EnumInfo**)array_size_check(info->Enums, sizeof(EnumInfo*), info->NumberOfEnums);
  info->Enums[info->NumberOfEnums++] = item;
}

/* Add a Constant ValueInfo to a ClassInfo */
void svtkParse_AddConstantToClass(ClassInfo* info, ValueInfo* item)
{
  svtkParse_AddItemToArray(
    &info->Items, &info->NumberOfItems, item->ItemType, info->NumberOfConstants);
  info->Constants =
    (ValueInfo**)array_size_check(info->Constants, sizeof(ValueInfo*), info->NumberOfConstants);
  info->Constants[info->NumberOfConstants++] = item;
}

/* Add a Variable ValueInfo to a ClassInfo */
void svtkParse_AddVariableToClass(ClassInfo* info, ValueInfo* item)
{
  svtkParse_AddItemToArray(
    &info->Items, &info->NumberOfItems, item->ItemType, info->NumberOfVariables);
  info->Variables =
    (ValueInfo**)array_size_check(info->Variables, sizeof(ValueInfo*), info->NumberOfVariables);
  info->Variables[info->NumberOfVariables++] = item;
}

/* Add a Typedef ValueInfo to a ClassInfo */
void svtkParse_AddTypedefToClass(ClassInfo* info, ValueInfo* item)
{
  svtkParse_AddItemToArray(
    &info->Items, &info->NumberOfItems, item->ItemType, info->NumberOfTypedefs);
  info->Typedefs =
    (ValueInfo**)array_size_check(info->Typedefs, sizeof(ValueInfo*), info->NumberOfTypedefs);
  info->Typedefs[info->NumberOfTypedefs++] = item;
}

/* Add a UsingInfo to a ClassInfo */
void svtkParse_AddUsingToClass(ClassInfo* info, UsingInfo* item)
{
  svtkParse_AddItemToArray(&info->Items, &info->NumberOfItems, item->ItemType, info->NumberOfUsings);
  info->Usings =
    (UsingInfo**)array_size_check(info->Usings, sizeof(UsingInfo*), info->NumberOfUsings);
  info->Usings[info->NumberOfUsings++] = item;
}

/* Add a CommentInfo to a ClassInfo */
void svtkParse_AddCommentToClass(ClassInfo* info, CommentInfo* item)
{
  info->Comments =
    (CommentInfo**)array_size_check(info->Comments, sizeof(CommentInfo*), info->NumberOfComments);
  info->Comments[info->NumberOfComments++] = item;
}

/* Add a NamespaceInfo to a NamespaceInfo */
void svtkParse_AddNamespaceToNamespace(NamespaceInfo* info, NamespaceInfo* item)
{
  svtkParse_AddItemToArray(
    &info->Items, &info->NumberOfItems, item->ItemType, info->NumberOfNamespaces);
  info->Namespaces = (NamespaceInfo**)array_size_check(
    info->Namespaces, sizeof(NamespaceInfo*), info->NumberOfNamespaces);
  info->Namespaces[info->NumberOfNamespaces++] = item;
}

/* Add a ClassInfo to a NamespaceInfo */
void svtkParse_AddClassToNamespace(NamespaceInfo* info, ClassInfo* item)
{
  svtkParse_AddClassToClass(info, item);
}

/* Add a FunctionInfo to a NamespaceInfo */
void svtkParse_AddFunctionToNamespace(NamespaceInfo* info, FunctionInfo* item)
{
  svtkParse_AddFunctionToClass(info, item);
}

/* Add a EnumInfo to a NamespaceInfo */
void svtkParse_AddEnumToNamespace(NamespaceInfo* info, EnumInfo* item)
{
  svtkParse_AddEnumToClass(info, item);
}

/* Add a Constant ValueInfo to a NamespaceInfo */
void svtkParse_AddConstantToNamespace(NamespaceInfo* info, ValueInfo* item)
{
  svtkParse_AddConstantToClass(info, item);
}

/* Add a Variable ValueInfo to a NamespaceInfo */
void svtkParse_AddVariableToNamespace(NamespaceInfo* info, ValueInfo* item)
{
  svtkParse_AddVariableToClass(info, item);
}

/* Add a Typedef ValueInfo to a NamespaceInfo */
void svtkParse_AddTypedefToNamespace(NamespaceInfo* info, ValueInfo* item)
{
  svtkParse_AddTypedefToClass(info, item);
}

/* Add a UsingInfo to a NamespaceInfo */
void svtkParse_AddUsingToNamespace(NamespaceInfo* info, UsingInfo* item)
{
  svtkParse_AddUsingToClass(info, item);
}

/* Add a CommentInfo to a NamespaceInfo */
void svtkParse_AddCommentToNamespace(NamespaceInfo* info, CommentInfo* item)
{
  svtkParse_AddCommentToClass(info, item);
}

/* Add a ValueInfo parameter to a FunctionInfo */
void svtkParse_AddParameterToFunction(FunctionInfo* info, ValueInfo* item)
{
  info->Parameters =
    (ValueInfo**)array_size_check(info->Parameters, sizeof(ValueInfo*), info->NumberOfParameters);
  info->Parameters[info->NumberOfParameters++] = item;
}

/* Add a ValueInfo to a TemplateInfo */
void svtkParse_AddParameterToTemplate(TemplateInfo* info, ValueInfo* item)
{
  info->Parameters =
    (ValueInfo**)array_size_check(info->Parameters, sizeof(ValueInfo*), info->NumberOfParameters);
  info->Parameters[info->NumberOfParameters++] = item;
}

/* Add default constructors if they do not already exist */
void svtkParse_AddDefaultConstructors(ClassInfo* cls, StringCache* cache)
{
  FunctionInfo* func;
  ValueInfo* param;
  size_t k, l;
  int i, n;
  int default_constructor = 1;
  int copy_constructor = 1;
  char* tname;
  const char* ccname;

  if (cls == NULL || cls->Name == NULL)
  {
    return;
  }

  n = cls->NumberOfFunctions;
  for (i = 0; i < n; i++)
  {
    func = cls->Functions[i];
    if (func->Name && strcmp(func->Name, cls->Name) == 0)
    {
      default_constructor = 0;

      if (func->NumberOfParameters == 1)
      {
        param = func->Parameters[0];
        if (param->Class && strcmp(param->Class, cls->Name) == 0 &&
          (param->Type & SVTK_PARSE_POINTER_MASK) == 0)
        {
          copy_constructor = 0;
        }
      }
    }
  }

  if (default_constructor)
  {
    func = (FunctionInfo*)malloc(sizeof(FunctionInfo));
    svtkParse_InitFunction(func);
    func->Class = cls->Name;
    func->Name = cls->Name;
    k = strlen(cls->Name);
    tname = svtkParse_NewString(cache, k + 2);
    strcpy(tname, cls->Name);
    strcpy(&tname[k], "()");
    func->Signature = tname;
    svtkParse_AddFunctionToClass(cls, func);
  }

  if (copy_constructor)
  {
    ccname = cls->Name;

    if (cls->Template)
    {
      /* specialize the name */
      n = cls->Template->NumberOfParameters;

      k = strlen(cls->Name) + 2;
      for (i = 0; i < n; i++)
      {
        if (cls->Template->Parameters[i]->Name)
        {
          k += strlen(cls->Template->Parameters[i]->Name) + 2;
        }
      }
      tname = svtkParse_NewString(cache, k);
      strcpy(tname, cls->Name);
      k = strlen(tname);
      tname[k++] = '<';
      for (i = 0; i < n; i++)
      {
        if (cls->Template->Parameters[i]->Name)
        {
          strcpy(&tname[k], cls->Template->Parameters[i]->Name);
          k += strlen(cls->Template->Parameters[i]->Name);
        }
        if (i + 1 < n)
        {
          tname[k++] = ',';
          tname[k++] = ' ';
        }
      }
      tname[k++] = '>';
      tname[k] = '\0';
      ccname = tname;
    }

    func = (FunctionInfo*)malloc(sizeof(FunctionInfo));
    svtkParse_InitFunction(func);
    func->Class = cls->Name;
    func->Name = cls->Name;
    k = strlen(cls->Name);
    l = strlen(ccname);
    tname = svtkParse_NewString(cache, k + l + 9);
    strcpy(tname, cls->Name);
    strcpy(&tname[k], "(const &");
    strcpy(&tname[k + 8], ccname);
    strcpy(&tname[k + 8 + l], ")");
    func->Signature = tname;
    param = (ValueInfo*)malloc(sizeof(ValueInfo));
    svtkParse_InitValue(param);
    param->Type = (SVTK_PARSE_OBJECT_REF | SVTK_PARSE_CONST);
    param->Class = ccname;
    svtkParse_AddParameterToFunction(func, param);
    svtkParse_AddFunctionToClass(cls, func);
  }
}
