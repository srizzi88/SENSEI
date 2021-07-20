/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkWrap.c

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#include "svtkWrap.h"
#include "svtkParseData.h"
#include "svtkParseExtras.h"
#include "svtkParseMain.h"
#include "svtkParseMerge.h"
#include "svtkParseString.h"

#include <ctype.h>
#include <stdlib.h>
#include <string.h>

/* -------------------------------------------------------------------- */
/* Common types. */

int svtkWrap_IsVoid(ValueInfo* val)
{
  if (val == 0)
  {
    return 1;
  }

  return ((val->Type & SVTK_PARSE_UNQUALIFIED_TYPE) == SVTK_PARSE_VOID);
}

int svtkWrap_IsVoidFunction(ValueInfo* val)
{
  unsigned int t = (val->Type & SVTK_PARSE_UNQUALIFIED_TYPE);

  if (t == SVTK_PARSE_FUNCTION_PTR || t == SVTK_PARSE_FUNCTION)
  {
    /* check for signature "void (*func)(void *)" */
    if (val->Function->NumberOfParameters == 1 &&
      val->Function->Parameters[0]->Type == SVTK_PARSE_VOID_PTR &&
      val->Function->Parameters[0]->NumberOfDimensions == 0 &&
      val->Function->ReturnValue->Type == SVTK_PARSE_VOID)
    {
      return 1;
    }
  }

  return 0;
}

int svtkWrap_IsVoidPointer(ValueInfo* val)
{
  unsigned int t = (val->Type & SVTK_PARSE_BASE_TYPE);
  return (t == SVTK_PARSE_VOID && svtkWrap_IsPointer(val));
}

int svtkWrap_IsCharPointer(ValueInfo* val)
{
  unsigned int t = (val->Type & SVTK_PARSE_BASE_TYPE);
  return (t == SVTK_PARSE_CHAR && svtkWrap_IsPointer(val) && (val->Type & SVTK_PARSE_ZEROCOPY) == 0);
}

int svtkWrap_IsPODPointer(ValueInfo* val)
{
  unsigned int t = (val->Type & SVTK_PARSE_BASE_TYPE);
  return (t != SVTK_PARSE_CHAR && svtkWrap_IsNumeric(val) && svtkWrap_IsPointer(val) &&
    (val->Type & SVTK_PARSE_ZEROCOPY) == 0);
}

int svtkWrap_IsZeroCopyPointer(ValueInfo* val)
{
  return (svtkWrap_IsPointer(val) && (val->Type & SVTK_PARSE_ZEROCOPY) != 0);
}

int svtkWrap_IsStdVector(ValueInfo* val)
{
  return ((val->Type & SVTK_PARSE_BASE_TYPE) == SVTK_PARSE_UNKNOWN && val->Class &&
    strncmp(val->Class, "std::vector<", 12) == 0);
}

int svtkWrap_IsSVTKObject(ValueInfo* val)
{
  unsigned int t = (val->Type & SVTK_PARSE_UNQUALIFIED_TYPE);
  return (t == SVTK_PARSE_OBJECT_PTR && !val->IsEnum && val->Class[0] == 'v' &&
    strncmp(val->Class, "svtk", 4) == 0);
}

int svtkWrap_IsSpecialObject(ValueInfo* val)
{
  unsigned int t = (val->Type & SVTK_PARSE_UNQUALIFIED_TYPE);
  return ((t == SVTK_PARSE_OBJECT || t == SVTK_PARSE_OBJECT_REF) && !val->IsEnum &&
    val->Class[0] == 'v' && strncmp(val->Class, "svtk", 4) == 0);
}

int svtkWrap_IsPythonObject(ValueInfo* val)
{
  unsigned int t = (val->Type & SVTK_PARSE_BASE_TYPE);
  return (t == SVTK_PARSE_UNKNOWN && strncmp(val->Class, "Py", 2) == 0);
}

/* -------------------------------------------------------------------- */
/* The base types, all are mutually exclusive. */

int svtkWrap_IsObject(ValueInfo* val)
{
  unsigned int t = (val->Type & SVTK_PARSE_BASE_TYPE);
  return (t == SVTK_PARSE_OBJECT || t == SVTK_PARSE_QOBJECT);
}

int svtkWrap_IsFunction(ValueInfo* val)
{
  unsigned int t = (val->Type & SVTK_PARSE_BASE_TYPE);
  return (t == SVTK_PARSE_FUNCTION);
}

int svtkWrap_IsStream(ValueInfo* val)
{
  unsigned int t = (val->Type & SVTK_PARSE_BASE_TYPE);
  return (t == SVTK_PARSE_ISTREAM || t == SVTK_PARSE_OSTREAM);
}

int svtkWrap_IsNumeric(ValueInfo* val)
{
  unsigned int t = (val->Type & SVTK_PARSE_BASE_TYPE);

  t = (t & ~SVTK_PARSE_UNSIGNED);
  switch (t)
  {
    case SVTK_PARSE_FLOAT:
    case SVTK_PARSE_DOUBLE:
    case SVTK_PARSE_CHAR:
    case SVTK_PARSE_SHORT:
    case SVTK_PARSE_INT:
    case SVTK_PARSE_LONG:
    case SVTK_PARSE_LONG_LONG:
    case SVTK_PARSE___INT64:
    case SVTK_PARSE_SIGNED_CHAR:
    case SVTK_PARSE_SSIZE_T:
    case SVTK_PARSE_BOOL:
      return 1;
  }

  return 0;
}

int svtkWrap_IsString(ValueInfo* val)
{
  unsigned int t = (val->Type & SVTK_PARSE_BASE_TYPE);
  return (t == SVTK_PARSE_STRING || t == SVTK_PARSE_UNICODE_STRING);
}

/* -------------------------------------------------------------------- */
/* Subcategories */

int svtkWrap_IsBool(ValueInfo* val)
{
  unsigned int t = (val->Type & SVTK_PARSE_BASE_TYPE);
  return (t == SVTK_PARSE_BOOL);
}

int svtkWrap_IsChar(ValueInfo* val)
{
  unsigned int t = (val->Type & SVTK_PARSE_BASE_TYPE);
  return (t == SVTK_PARSE_CHAR);
}

int svtkWrap_IsInteger(ValueInfo* val)
{
  unsigned int t = (val->Type & SVTK_PARSE_BASE_TYPE);

  if (t != SVTK_PARSE_UNSIGNED_CHAR)
  {
    t = (t & ~SVTK_PARSE_UNSIGNED);
  }
  switch (t)
  {
    case SVTK_PARSE_SHORT:
    case SVTK_PARSE_INT:
    case SVTK_PARSE_LONG:
    case SVTK_PARSE_LONG_LONG:
    case SVTK_PARSE___INT64:
    case SVTK_PARSE_UNSIGNED_CHAR:
    case SVTK_PARSE_SIGNED_CHAR:
    case SVTK_PARSE_SSIZE_T:
      return 1;
  }

  return 0;
}

int svtkWrap_IsRealNumber(ValueInfo* val)
{
  unsigned int t = (val->Type & SVTK_PARSE_BASE_TYPE);
  return (t == SVTK_PARSE_FLOAT || t == SVTK_PARSE_DOUBLE);
}

/* -------------------------------------------------------------------- */
/* These are mutually exclusive, as well. */

int svtkWrap_IsScalar(ValueInfo* val)
{
  unsigned int i = (val->Type & SVTK_PARSE_POINTER_MASK);
  return (i == 0);
}

int svtkWrap_IsPointer(ValueInfo* val)
{
  unsigned int i = (val->Type & SVTK_PARSE_POINTER_MASK);
  return (i == SVTK_PARSE_POINTER && val->Count == 0 && val->CountHint == 0 &&
    val->NumberOfDimensions <= 1);
}

int svtkWrap_IsArray(ValueInfo* val)
{
  unsigned int i = (val->Type & SVTK_PARSE_POINTER_MASK);
  return (i == SVTK_PARSE_POINTER && val->NumberOfDimensions <= 1 &&
    (val->Count != 0 || val->CountHint != 0));
}

int svtkWrap_IsNArray(ValueInfo* val)
{
  int j = 0;
  unsigned int i = (val->Type & SVTK_PARSE_POINTER_MASK);
  if (i != SVTK_PARSE_ARRAY || val->NumberOfDimensions <= 1)
  {
    return 0;
  }
  for (j = 0; j < val->NumberOfDimensions; j++)
  {
    if (val->Dimensions[j] == NULL || val->Dimensions[j][0] == '\0')
    {
      return 0;
    }
  }
  return 1;
}

/* -------------------------------------------------------------------- */
/* Other type properties, not mutually exclusive. */

int svtkWrap_IsNonConstRef(ValueInfo* val)
{
  int isconst = ((val->Type & SVTK_PARSE_CONST) != 0);
  unsigned int ptrBits = val->Type & SVTK_PARSE_POINTER_MASK;

  /* If this is a reference to a pointer, we need to check whether
   * the pointer is const, for example "int *const &arg".  The "const"
   * we need to check is the one that is adjacent to the "&". */
  while (ptrBits != 0)
  {
    isconst = ((ptrBits & SVTK_PARSE_POINTER_LOWMASK) == SVTK_PARSE_CONST_POINTER);
    ptrBits >>= 2;
  }

  return ((val->Type & SVTK_PARSE_REF) != 0 && !isconst);
}

int svtkWrap_IsConstRef(ValueInfo* val)
{
  return ((val->Type & SVTK_PARSE_REF) != 0 && !svtkWrap_IsNonConstRef(val));
}

int svtkWrap_IsRef(ValueInfo* val)
{
  return ((val->Type & SVTK_PARSE_REF) != 0);
}

int svtkWrap_IsConst(ValueInfo* val)
{
  return ((val->Type & SVTK_PARSE_CONST) != 0);
}

/* -------------------------------------------------------------------- */
/* Check if the arg type is an enum that is a member of the class */
int svtkWrap_IsEnumMember(ClassInfo* data, ValueInfo* arg)
{
  int i;

  if (arg->Class)
  {
    /* check if the enum is a member of the class */
    for (i = 0; i < data->NumberOfEnums; i++)
    {
      EnumInfo* info = data->Enums[i];
      if (info->Name && strcmp(arg->Class, info->Name) == 0)
      {
        return 1;
      }
    }
  }

  return 0;
}

/* -------------------------------------------------------------------- */
/* Hints */

int svtkWrap_IsNewInstance(ValueInfo* val)
{
  return ((val->Type & SVTK_PARSE_NEWINSTANCE) != 0);
}

/* -------------------------------------------------------------------- */
/* Constructor/Destructor checks */

int svtkWrap_IsConstructor(ClassInfo* c, FunctionInfo* f)

{
  size_t i, m;
  const char* cp = c->Name;

  if (cp && f->Name && !svtkWrap_IsDestructor(c, f))
  {
    /* remove namespaces and template parameters from the name */
    m = svtkParse_UnscopedNameLength(cp);
    while (cp[m] == ':' && cp[m + 1] == ':')
    {
      cp += m + 2;
      m = svtkParse_UnscopedNameLength(cp);
    }
    for (i = 0; i < m; i++)
    {
      if (cp[i] == '<')
      {
        break;
      }
    }

    return (i == strlen(f->Name) && strncmp(cp, f->Name, i) == 0);
  }

  return 0;
}

int svtkWrap_IsDestructor(ClassInfo* c, FunctionInfo* f)
{
  size_t i;
  const char* cp;

  if (c->Name && f->Name)
  {
    cp = f->Signature;
    for (i = 0; cp[i] != '\0' && cp[i] != '('; i++)
    {
      if (cp[i] == '~')
      {
        return 1;
      }
    }
  }

  return 0;
}

int svtkWrap_IsInheritedMethod(ClassInfo* c, FunctionInfo* f)
{
  size_t l;
  for (l = 0; c->Name[l]; l++)
  {
    /* ignore template args */
    if (c->Name[l] == '<')
    {
      break;
    }
  }

  if (f->Class && (strlen(f->Class) != l || strncmp(f->Class, c->Name, l) != 0))
  {
    return 1;
  }

  return 0;
}

int svtkWrap_IsSetVectorMethod(FunctionInfo* f)
{
  if (f->Macro && strncmp(f->Macro, "svtkSetVector", 13) == 0)
  {
    return 1;
  }

  return 0;
}

int svtkWrap_IsGetVectorMethod(FunctionInfo* f)
{
  if (f->Macro && strncmp(f->Macro, "svtkGetVector", 13) == 0)
  {
    return 1;
  }

  return 0;
}

/* -------------------------------------------------------------------- */
/* Argument counting */

int svtkWrap_CountWrappedParameters(FunctionInfo* f)
{
  int totalArgs = f->NumberOfParameters;

  if (totalArgs > 0 && (f->Parameters[0]->Type & SVTK_PARSE_BASE_TYPE) == SVTK_PARSE_FUNCTION)
  {
    totalArgs = 1;
  }
  else if (totalArgs == 1 &&
    (f->Parameters[0]->Type & SVTK_PARSE_UNQUALIFIED_TYPE) == SVTK_PARSE_VOID)
  {
    totalArgs = 0;
  }

  return totalArgs;
}

int svtkWrap_CountRequiredArguments(FunctionInfo* f)
{
  int requiredArgs = 0;
  int totalArgs;
  int i;

  totalArgs = svtkWrap_CountWrappedParameters(f);

  for (i = 0; i < totalArgs; i++)
  {
    if (f->Parameters[i]->Value == NULL || svtkWrap_IsNArray(f->Parameters[i]))
    {
      requiredArgs = i + 1;
    }
  }

  return requiredArgs;
}

/* -------------------------------------------------------------------- */
/* Check whether the class is derived from svtkObjectBase. */

int svtkWrap_IsSVTKObjectBaseType(HierarchyInfo* hinfo, const char* classname)
{
  HierarchyEntry* entry;

  if (hinfo)
  {
    entry = svtkParseHierarchy_FindEntry(hinfo, classname);
    if (entry)
    {
      if (svtkParseHierarchy_IsTypeOf(hinfo, entry, "svtkObjectBase"))
      {
        return 1;
      }
      return 0;
    }
  }

  /* fallback if no HierarchyInfo, but skip smart pointers */
  if (strncmp("svtk", classname, 4) == 0 && strncmp("svtkSmartPointer", classname, 16) != 0)
  {
    return 1;
  }

  return 0;
}

/* -------------------------------------------------------------------- */
/* Check if the class is not derived from svtkObjectBase. */

int svtkWrap_IsSpecialType(HierarchyInfo* hinfo, const char* classname)
{
  HierarchyEntry* entry;

  if (hinfo)
  {
    entry = svtkParseHierarchy_FindEntry(hinfo, classname);
    if (entry)
    {
      if (!svtkParseHierarchy_IsTypeOf(hinfo, entry, "svtkObjectBase"))
      {
        return 1;
      }
    }
    return 0;
  }

  /* fallback if no HierarchyInfo */
  if (strncmp("svtk", classname, 4) == 0)
  {
    return -1;
  }

  return 0;
}

/* -------------------------------------------------------------------- */
/* Check if the class is derived from superclass */

int svtkWrap_IsTypeOf(HierarchyInfo* hinfo, const char* classname, const char* superclass)
{
  HierarchyEntry* entry;

  if (strcmp(classname, superclass) == 0)
  {
    return 1;
  }

  if (hinfo)
  {
    entry = svtkParseHierarchy_FindEntry(hinfo, classname);
    if (entry && svtkParseHierarchy_IsTypeOf(hinfo, entry, superclass))
    {
      return 1;
    }
  }

  return 0;
}

/* -------------------------------------------------------------------- */
/* Make a guess about whether a class is wrapped */

int svtkWrap_IsClassWrapped(HierarchyInfo* hinfo, const char* classname)
{
  if (hinfo)
  {
    HierarchyEntry* entry;
    entry = svtkParseHierarchy_FindEntry(hinfo, classname);

    if (entry)
    {
      return 1;
    }
  }
  else if (strncmp("svtk", classname, 4) == 0)
  {
    return 1;
  }

  return 0;
}

/* -------------------------------------------------------------------- */
/* Check whether the destructor is public */
int svtkWrap_HasPublicDestructor(ClassInfo* data)
{
  FunctionInfo* func;
  int i;

  for (i = 0; i < data->NumberOfFunctions; i++)
  {
    func = data->Functions[i];

    if (svtkWrap_IsDestructor(data, func) && (func->Access != SVTK_ACCESS_PUBLIC || func->IsDeleted))
    {
      return 0;
    }
  }

  return 1;
}

/* -------------------------------------------------------------------- */
/* Check whether the copy constructor is public */
int svtkWrap_HasPublicCopyConstructor(ClassInfo* data)
{
  FunctionInfo* func;
  int i;

  for (i = 0; i < data->NumberOfFunctions; i++)
  {
    func = data->Functions[i];

    if (svtkWrap_IsConstructor(data, func) && func->NumberOfParameters == 1 &&
      func->Parameters[0]->Class && strcmp(func->Parameters[0]->Class, data->Name) == 0 &&
      (func->Access != SVTK_ACCESS_PUBLIC || func->IsDeleted))
    {
      return 0;
    }
  }

  return 1;
}

/* -------------------------------------------------------------------- */
/* Get the size for subclasses of svtkTuple */
int svtkWrap_GetTupleSize(ClassInfo* data, HierarchyInfo* hinfo)
{
  HierarchyEntry* entry;
  const char* classname = NULL;
  size_t m;
  int size = 0;

  entry = svtkParseHierarchy_FindEntry(hinfo, data->Name);
  if (entry &&
    svtkParseHierarchy_IsTypeOfTemplated(hinfo, entry, data->Name, "svtkTuple", &classname))
  {
    /* attempt to get count from template parameter */
    if (classname)
    {
      m = strlen(classname);
      if (m > 2 && classname[m - 1] == '>' && isdigit(classname[m - 2]) &&
        (classname[m - 3] == ' ' || classname[m - 3] == ',' || classname[m - 3] == '<'))
      {
        size = classname[m - 2] - '0';
      }
      free((char*)classname);
    }
  }

  return size;
}

/* -------------------------------------------------------------------- */
/* This sets the CountHint for svtkDataArray methods where the
 * tuple size is equal to GetNumberOfComponents. */
void svtkWrap_FindCountHints(ClassInfo* data, FileInfo* finfo, HierarchyInfo* hinfo)
{
  int i;
  int count;
  const char* countMethod;
  FunctionInfo* theFunc;

  /* add hints for svtkInformation get methods */
  if (svtkWrap_IsTypeOf(hinfo, data->Name, "svtkInformation"))
  {
    countMethod = "Length(temp0)";

    for (i = 0; i < data->NumberOfFunctions; i++)
    {
      theFunc = data->Functions[i];

      if (strcmp(theFunc->Name, "Get") == 0 && theFunc->NumberOfParameters >= 1 &&
        theFunc->Parameters[0]->Type == SVTK_PARSE_OBJECT_PTR &&
        (strcmp(theFunc->Parameters[0]->Class, "svtkInformationIntegerVectorKey") == 0 ||
          strcmp(theFunc->Parameters[0]->Class, "svtkInformationDoubleVectorKey") == 0))
      {
        if (theFunc->ReturnValue && theFunc->ReturnValue->Count == 0 &&
          theFunc->NumberOfParameters == 1)
        {
          theFunc->ReturnValue->CountHint = countMethod;
        }
      }
    }
  }

  /* add hints for array GetTuple methods */
  if (svtkWrap_IsTypeOf(hinfo, data->Name, "svtkDataArray") ||
    svtkWrap_IsTypeOf(hinfo, data->Name, "svtkArrayIterator"))
  {
    countMethod = "GetNumberOfComponents()";

    for (i = 0; i < data->NumberOfFunctions; i++)
    {
      theFunc = data->Functions[i];

      if ((strcmp(theFunc->Name, "GetTuple") == 0 || strcmp(theFunc->Name, "GetTypedTuple") == 0) &&
        theFunc->ReturnValue && theFunc->ReturnValue->Count == 0 &&
        theFunc->NumberOfParameters == 1 && svtkWrap_IsScalar(theFunc->Parameters[0]) &&
        svtkWrap_IsInteger(theFunc->Parameters[0]))
      {
        theFunc->ReturnValue->CountHint = countMethod;
      }
      else if ((strcmp(theFunc->Name, "SetTuple") == 0 ||
                 strcmp(theFunc->Name, "SetTypedTuple") == 0 ||
                 strcmp(theFunc->Name, "GetTuple") == 0 ||
                 strcmp(theFunc->Name, "GetTypedTuple") == 0 ||
                 strcmp(theFunc->Name, "InsertTuple") == 0 ||
                 strcmp(theFunc->Name, "InsertTypedTuple") == 0) &&
        theFunc->NumberOfParameters == 2 && svtkWrap_IsScalar(theFunc->Parameters[0]) &&
        svtkWrap_IsInteger(theFunc->Parameters[0]) && theFunc->Parameters[1]->Count == 0)
      {
        theFunc->Parameters[1]->CountHint = countMethod;
      }
      else if ((strcmp(theFunc->Name, "InsertNextTuple") == 0 ||
                 strcmp(theFunc->Name, "InsertNextTypedTuple") == 0) &&
        theFunc->NumberOfParameters == 1 && theFunc->Parameters[0]->Count == 0)
      {
        theFunc->Parameters[0]->CountHint = countMethod;
      }
    }
  }

  /* add hints for interpolator Interpolate methods */
  if (svtkWrap_IsTypeOf(hinfo, data->Name, "svtkAbstractImageInterpolator"))
  {
    countMethod = "GetNumberOfComponents()";

    for (i = 0; i < data->NumberOfFunctions; i++)
    {
      theFunc = data->Functions[i];

      if (strcmp(theFunc->Name, "Interpolate") == 0 && theFunc->NumberOfParameters == 2 &&
        theFunc->Parameters[0]->Type == (SVTK_PARSE_DOUBLE_PTR | SVTK_PARSE_CONST) &&
        theFunc->Parameters[0]->Count == 3 &&
        theFunc->Parameters[1]->Type == SVTK_PARSE_DOUBLE_PTR && theFunc->Parameters[1]->Count == 0)
      {
        theFunc->Parameters[1]->CountHint = countMethod;
      }
    }
  }

  for (i = 0; i < data->NumberOfFunctions; i++)
  {
    theFunc = data->Functions[i];

    /* hints for constructors that take arrays */
    if (svtkWrap_IsConstructor(data, theFunc) && theFunc->NumberOfParameters == 1 &&
      svtkWrap_IsPointer(theFunc->Parameters[0]) && svtkWrap_IsNumeric(theFunc->Parameters[0]) &&
      theFunc->Parameters[0]->Count == 0 && hinfo)
    {
      count = svtkWrap_GetTupleSize(data, hinfo);
      if (count)
      {
        char counttext[24];
        sprintf(counttext, "%d", count);
        theFunc->Parameters[0]->Count = count;
        svtkParse_AddStringToArray(&theFunc->Parameters[0]->Dimensions,
          &theFunc->Parameters[0]->NumberOfDimensions,
          svtkParse_CacheString(finfo->Strings, counttext, strlen(counttext)));
      }
    }

    /* hints for operator[] index range */
    if (theFunc->IsOperator && theFunc->Name && strcmp(theFunc->Name, "operator[]") == 0)
    {
      if (svtkWrap_IsTypeOf(hinfo, data->Name, "svtkTuple"))
      {
        theFunc->SizeHint = "GetSize()";
      }
      else if (svtkWrap_IsTypeOf(hinfo, data->Name, "svtkArrayCoordinates") ||
        svtkWrap_IsTypeOf(hinfo, data->Name, "svtkArrayExtents") ||
        svtkWrap_IsTypeOf(hinfo, data->Name, "svtkArraySort"))
      {
        theFunc->SizeHint = "GetDimensions()";
      }
      else if (svtkWrap_IsTypeOf(hinfo, data->Name, "svtkArrayExtentsList") ||
        svtkWrap_IsTypeOf(hinfo, data->Name, "svtkArrayWeights"))
      {
        theFunc->SizeHint = "GetCount()";
      }
    }
  }
}

/* -------------------------------------------------------------------- */
/* This sets the NewInstance hint for generator methods. */
void svtkWrap_FindNewInstanceMethods(ClassInfo* data, HierarchyInfo* hinfo)
{
  int i;
  FunctionInfo* theFunc;
  OptionInfo* options;

  for (i = 0; i < data->NumberOfFunctions; i++)
  {
    theFunc = data->Functions[i];
    if (theFunc->Name && theFunc->ReturnValue && svtkWrap_IsSVTKObject(theFunc->ReturnValue) &&
      svtkWrap_IsSVTKObjectBaseType(hinfo, theFunc->ReturnValue->Class))
    {
      if (strcmp(theFunc->Name, "NewInstance") == 0 || strcmp(theFunc->Name, "NewIterator") == 0 ||
        strcmp(theFunc->Name, "CreateInstance") == 0)
      {
        if ((theFunc->ReturnValue->Type & SVTK_PARSE_NEWINSTANCE) == 0)
        {
          /* get the command-line options */
          options = svtkParse_GetCommandLineOptions();
          fprintf(stderr, "Warning: %s without SVTK_NEWINSTANCE hint in %s\n", theFunc->Name,
            options->InputFileName);
          theFunc->ReturnValue->Type |= SVTK_PARSE_NEWINSTANCE;
        }
      }
    }
  }
}

/* -------------------------------------------------------------------- */
/* Expand all typedef types that are used in function arguments */
void svtkWrap_ExpandTypedefs(ClassInfo* data, FileInfo* finfo, HierarchyInfo* hinfo)
{
  int i, j, n;
  FunctionInfo* funcInfo;
  const char* newclass;

  n = data->NumberOfSuperClasses;
  for (i = 0; i < n; i++)
  {
    newclass = svtkParseHierarchy_ExpandTypedefsInName(hinfo, data->SuperClasses[i], NULL);
    if (newclass != data->SuperClasses[i])
    {
      data->SuperClasses[i] = svtkParse_CacheString(finfo->Strings, newclass, strlen(newclass));
      free((char*)newclass);
    }
  }

  n = data->NumberOfFunctions;
  for (i = 0; i < n; i++)
  {
    funcInfo = data->Functions[i];
    if (funcInfo->Access == SVTK_ACCESS_PUBLIC)
    {
      for (j = 0; j < funcInfo->NumberOfParameters; j++)
      {
        svtkParseHierarchy_ExpandTypedefsInValue(
          hinfo, funcInfo->Parameters[j], finfo->Strings, funcInfo->Class);
#ifndef SVTK_PARSE_LEGACY_REMOVE
        if (j < MAX_ARGS)
        {
          if (svtkWrap_IsFunction(funcInfo->Parameters[j]))
          {
            // legacy args only allow "void func(void *)" functions
            if (svtkWrap_IsVoidFunction(funcInfo->Parameters[j]))
            {
              funcInfo->ArgTypes[j] = SVTK_PARSE_FUNCTION;
              funcInfo->ArgClasses[j] = funcInfo->Parameters[j]->Class;
            }
          }
          else
          {
            funcInfo->ArgTypes[j] = funcInfo->Parameters[j]->Type;
            funcInfo->ArgClasses[j] = funcInfo->Parameters[j]->Class;
          }
        }
#endif
      }
      if (funcInfo->ReturnValue)
      {
        svtkParseHierarchy_ExpandTypedefsInValue(
          hinfo, funcInfo->ReturnValue, finfo->Strings, funcInfo->Class);
#ifndef SVTK_PARSE_LEGACY_REMOVE
        if (!svtkWrap_IsFunction(funcInfo->ReturnValue))
        {
          funcInfo->ReturnType = funcInfo->ReturnValue->Type;
          funcInfo->ReturnClass = funcInfo->ReturnValue->Class;
        }
#endif
      }
    }
  }
}

/* -------------------------------------------------------------------- */
/* Merge superclass methods according to using declarations */
void svtkWrap_ApplyUsingDeclarations(ClassInfo* data, FileInfo* finfo, HierarchyInfo* hinfo)
{
  int i, n;

  /* first, check if there are any declarations to apply */
  n = data->NumberOfUsings;
  for (i = 0; i < n; i++)
  {
    if (data->Usings[i]->Name)
    {
      break;
    }
  }
  /* if using declarations found, read superclass headers */
  if (i < n)
  {
    n = data->NumberOfSuperClasses;
    for (i = 0; i < n; i++)
    {
      svtkParseMerge_MergeHelper(
        finfo, finfo->Contents, hinfo, data->SuperClasses[i], 0, NULL, NULL, data);
    }
  }
}

/* -------------------------------------------------------------------- */
/* Merge superclass methods */
void svtkWrap_MergeSuperClasses(ClassInfo* data, FileInfo* finfo, HierarchyInfo* hinfo)
{
  int n = data->NumberOfSuperClasses;
  int i;
  MergeInfo* info;

  if (n == 0)
  {
    return;
  }

  info = svtkParseMerge_CreateMergeInfo(data);

  for (i = 0; i < n; i++)
  {
    svtkParseMerge_MergeHelper(
      finfo, finfo->Contents, hinfo, data->SuperClasses[i], 0, NULL, info, data);
  }

  svtkParseMerge_FreeMergeInfo(info);
}

/* -------------------------------------------------------------------- */
/* get the type name */

const char* svtkWrap_GetTypeName(ValueInfo* val)
{
  unsigned int aType = val->Type;
  const char* aClass = val->Class;

  /* print the type itself */
  switch (aType & SVTK_PARSE_BASE_TYPE)
  {
    case SVTK_PARSE_FLOAT:
      return "float";
    case SVTK_PARSE_DOUBLE:
      return "double";
    case SVTK_PARSE_INT:
      return "int";
    case SVTK_PARSE_SHORT:
      return "short";
    case SVTK_PARSE_LONG:
      return "long";
    case SVTK_PARSE_VOID:
      return "void ";
    case SVTK_PARSE_CHAR:
      return "char";
    case SVTK_PARSE_UNSIGNED_INT:
      return "unsigned int";
    case SVTK_PARSE_UNSIGNED_SHORT:
      return "unsigned short";
    case SVTK_PARSE_UNSIGNED_LONG:
      return "unsigned long";
    case SVTK_PARSE_UNSIGNED_CHAR:
      return "unsigned char";
    case SVTK_PARSE_LONG_LONG:
      return "long long";
    case SVTK_PARSE___INT64:
      return "__int64";
    case SVTK_PARSE_UNSIGNED_LONG_LONG:
      return "unsigned long long";
    case SVTK_PARSE_UNSIGNED___INT64:
      return "unsigned __int64";
    case SVTK_PARSE_SIGNED_CHAR:
      return "signed char";
    case SVTK_PARSE_BOOL:
      return "bool";
    case SVTK_PARSE_UNICODE_STRING:
      return "svtkUnicodeString";
    case SVTK_PARSE_SSIZE_T:
      return "ssize_t";
    case SVTK_PARSE_SIZE_T:
      return "size_t";
  }

  return aClass;
}

/* -------------------------------------------------------------------- */
/* variable declarations */

void svtkWrap_DeclareVariable(
  FILE* fp, ClassInfo* data, ValueInfo* val, const char* name, int i, int flags)
{
  unsigned int aType;
  int j;
  const char* typeName;
  char* newTypeName = NULL;

  if (val == NULL)
  {
    return;
  }

  aType = (val->Type & SVTK_PARSE_UNQUALIFIED_TYPE);

  /* do nothing for void */
  if (aType == SVTK_PARSE_VOID || (aType & SVTK_PARSE_BASE_TYPE) == SVTK_PARSE_FUNCTION)
  {
    return;
  }

  typeName = svtkWrap_GetTypeName(val);

  if (svtkWrap_IsEnumMember(data, val))
  {
    /* use a typedef to work around compiler issues when someone used
       the same name for the enum type as for a variable or method */
    newTypeName = (char*)malloc(strlen(name) + 16);
    if (i >= 0)
    {
      sprintf(newTypeName, "%s%i_type", name, i);
    }
    else
    {
      sprintf(newTypeName, "%s_type", name);
    }
    fprintf(fp, "  typedef %s::%s %s;\n", data->Name, typeName, newTypeName);
    typeName = newTypeName;
  }

  /* add a couple spaces for indentation*/
  fprintf(fp, "  ");

  /* for const * return types, prepend with const */
  if ((flags & SVTK_WRAP_RETURN) != 0)
  {
    if ((val->Type & SVTK_PARSE_CONST) != 0 && (aType & SVTK_PARSE_INDIRECT) != 0)
    {
      fprintf(fp, "const ");
    }
  }
  /* do the same for "const char *" arguments */
  else
  {
    if ((val->Type & SVTK_PARSE_CONST) != 0 && aType == SVTK_PARSE_CHAR_PTR)
    {
      fprintf(fp, "const ");
    }
  }

  /* print the type name */
  fprintf(fp, "%s ", typeName);

  /* indirection */
  if ((flags & SVTK_WRAP_RETURN) != 0)
  {
    /* ref and pointer return values are stored as pointers */
    if ((aType & SVTK_PARSE_INDIRECT) == SVTK_PARSE_POINTER ||
      (aType & SVTK_PARSE_INDIRECT) == SVTK_PARSE_REF)
    {
      fprintf(fp, "*");
    }
  }
  else
  {
    /* objects refs and pointers are always handled via pointers,
     * other refs are passed by value */
    if (aType == SVTK_PARSE_CHAR_PTR || aType == SVTK_PARSE_VOID_PTR ||
      (!val->IsEnum &&
        (aType == SVTK_PARSE_OBJECT_PTR || aType == SVTK_PARSE_OBJECT_REF ||
          aType == SVTK_PARSE_OBJECT)))
    {
      fprintf(fp, "*");
    }
    /* arrays of unknown size are handled via pointers */
    else if (val->CountHint || svtkWrap_IsPODPointer(val) || svtkWrap_IsZeroCopyPointer(val) ||
      (svtkWrap_IsArray(val) && val->Value))
    {
      fprintf(fp, "*");
    }
  }

  /* the variable name */
  if (i >= 0)
  {
    fprintf(fp, "%s%i", name, i);
  }
  else
  {
    fprintf(fp, "%s", name);
  }

  if ((flags & SVTK_WRAP_ARG) != 0)
  {
    /* print the array decorators */
    if (((aType & SVTK_PARSE_POINTER_MASK) != 0) && aType != SVTK_PARSE_CHAR_PTR &&
      aType != SVTK_PARSE_VOID_PTR && aType != SVTK_PARSE_OBJECT_PTR && val->CountHint == NULL &&
      !svtkWrap_IsPODPointer(val) && !(svtkWrap_IsArray(val) && val->Value))
    {
      if (val->NumberOfDimensions <= 1 && val->Count > 0)
      {
        fprintf(fp, "[%d]", val->Count);
      }
      else
      {
        for (j = 0; j < val->NumberOfDimensions; j++)
        {
          fprintf(fp, "[%s]", val->Dimensions[j]);
        }
      }
    }

    /* add a default value */
    else if (val->Value)
    {
      fprintf(fp, " = ");
      svtkWrap_QualifyExpression(fp, data, val->Value);
    }
    else if (aType == SVTK_PARSE_CHAR_PTR || aType == SVTK_PARSE_VOID_PTR ||
      (!val->IsEnum &&
        (aType == SVTK_PARSE_OBJECT_PTR || aType == SVTK_PARSE_OBJECT_REF ||
          aType == SVTK_PARSE_OBJECT)))
    {
      fprintf(fp, " = nullptr");
    }
    else if (val->CountHint || svtkWrap_IsPODPointer(val))
    {
      fprintf(fp, " = nullptr");
    }
    else if (aType == SVTK_PARSE_BOOL)
    {
      fprintf(fp, " = false");
    }
  }

  /* finish off with a semicolon */
  if ((flags & SVTK_WRAP_NOSEMI) == 0)
  {
    fprintf(fp, ";\n");
  }

  free(newTypeName);
}

void svtkWrap_DeclareVariableSize(FILE* fp, ValueInfo* val, const char* name, int i)
{
  char idx[32];
  int j;

  idx[0] = '\0';
  if (i >= 0)
  {
    sprintf(idx, "%d", i);
  }

  if (val->NumberOfDimensions > 1)
  {
    fprintf(fp, "  static size_t %s%s[%d] = ", name, idx, val->NumberOfDimensions);

    for (j = 0; j < val->NumberOfDimensions; j++)
    {
      fprintf(fp, "%c %s", ((j == 0) ? '{' : ','), val->Dimensions[j]);
    }

    fprintf(fp, " };\n");
  }
  else if (val->Count != 0 || val->CountHint || svtkWrap_IsPODPointer(val))
  {
    fprintf(fp, "  %ssize_t %s%s = %d;\n", ((val->Count == 0 || val->Value != 0) ? "" : "const "),
      name, idx, (val->Count == 0 ? 0 : val->Count));
  }
  else if (val->NumberOfDimensions == 1)
  {
    fprintf(fp, "  const size_t %s%s = %s;\n", name, idx, val->Dimensions[0]);
  }
}

void svtkWrap_QualifyExpression(FILE* fp, ClassInfo* data, const char* text)
{
  StringTokenizer t;
  int qualified = 0;
  int matched;
  int j;

  /* tokenize the text according to C/C++ rules */
  svtkParse_InitTokenizer(&t, text, WS_DEFAULT);
  do
  {
    /* check whether we have found an unqualified identifier */
    matched = 0;
    if (t.tok == TOK_ID && !qualified)
    {
      /* check for class members */
      for (j = 0; j < data->NumberOfItems; j++)
      {
        ItemInfo* item = &data->Items[j];
        const char* name = NULL;

        if (item->Type == SVTK_CONSTANT_INFO)
        {
          /* enum values and other constants */
          name = data->Constants[item->Index]->Name;
        }
        else if (item->Type == SVTK_CLASS_INFO || item->Type == SVTK_STRUCT_INFO ||
          item->Type == SVTK_UNION_INFO)
        {
          /* embedded classes */
          name = data->Classes[item->Index]->Name;
        }
        else if (item->Type == SVTK_ENUM_INFO)
        {
          /* enum type */
          name = data->Enums[item->Index]->Name;
        }
        else if (item->Type == SVTK_TYPEDEF_INFO)
        {
          /* typedef'd type */
          name = data->Typedefs[item->Index]->Name;
        }

        if (name && strlen(name) == t.len && strncmp(name, t.text, t.len) == 0)
        {
          fprintf(fp, "%s::%s", data->Name, name);
          matched = 1;
          break;
        }
      }
    }

    if (!matched)
    {
      fprintf(fp, "%*.*s", (int)t.len, (int)t.len, t.text);
    }

    /* if next character is whitespace, add a space */
    if (svtkParse_CharType(t.text[t.len], CPRE_WHITE))
    {
      fprintf(fp, " ");
    }

    /* check whether the next identifier is qualified */
    qualified = (t.tok == TOK_SCOPE || t.tok == TOK_ARROW || t.tok == '.');
  } while (svtkParse_NextToken(&t));
}

char* svtkWrap_SafeSuperclassName(const char* name)
{
  int template_class = 0;
  size_t size = strlen(name);
  char* safe_name = malloc(size + 1);
  size_t i;

  memcpy(safe_name, name, size + 1);

  for (i = 0; i < size; ++i)
  {
    char c = name[i];
    if (c == '<' || c == '>')
    {
      safe_name[i] = '_';
      template_class = 1;
    }
    if (c == ',' || c == ' ')
    {
      safe_name[i] = '_';
    }
  }

  if (!template_class)
  {
    free(safe_name);
    return NULL;
  }
  return safe_name;
}
