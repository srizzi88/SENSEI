/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkParseJava.c

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#include "svtkParse.h"
#include "svtkParseHierarchy.h"
#include "svtkParseMain.h"
#include "svtkWrap.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

HierarchyInfo* hierarchyInfo = NULL;
StringCache* stringCache = NULL;
int numberOfWrappedFunctions = 0;
FunctionInfo* wrappedFunctions[1000];
FunctionInfo* currentFunction;

void output_temp(FILE* fp, int i)
{
  unsigned int aType = (currentFunction->ArgTypes[i] & SVTK_PARSE_UNQUALIFIED_TYPE);

  /* ignore void */
  if (aType == SVTK_PARSE_VOID)
  {
    return;
  }

  if (currentFunction->ArgTypes[i] == SVTK_PARSE_FUNCTION)
  {
    fprintf(fp, "Object id0, String id1");
    return;
  }

  if ((aType == SVTK_PARSE_CHAR_PTR) || (aType == SVTK_PARSE_STRING) ||
    (aType == SVTK_PARSE_STRING_REF))
  {
    fprintf(fp, "String ");
  }
  else
  {
    switch ((aType & SVTK_PARSE_BASE_TYPE) & ~SVTK_PARSE_UNSIGNED)
    {
      case SVTK_PARSE_FLOAT:
        fprintf(fp, "double ");
        break;
      case SVTK_PARSE_DOUBLE:
        fprintf(fp, "double ");
        break;
      case SVTK_PARSE_INT:
        fprintf(fp, "int ");
        break;
      case SVTK_PARSE_SHORT:
        fprintf(fp, "int ");
        break;
      case SVTK_PARSE_LONG:
        fprintf(fp, "int ");
        break;
      case SVTK_PARSE_LONG_LONG:
        fprintf(fp, "int ");
        break;
      case SVTK_PARSE___INT64:
        fprintf(fp, "int ");
        break;
      case SVTK_PARSE_SIGNED_CHAR:
        fprintf(fp, "char ");
        break;
      case SVTK_PARSE_BOOL:
        fprintf(fp, "boolean ");
        break;
      case SVTK_PARSE_VOID:
        fprintf(fp, "void ");
        break;
      case SVTK_PARSE_CHAR:
        fprintf(fp, "char ");
        break;
      case SVTK_PARSE_OBJECT:
        fprintf(fp, "%s ", currentFunction->ArgClasses[i]);
        break;
      case SVTK_PARSE_UNKNOWN:
        fprintf(fp, "int ");
        break;
    }
  }

  fprintf(fp, "id%i", i);
  if (((aType & SVTK_PARSE_INDIRECT) == SVTK_PARSE_POINTER) && (aType != SVTK_PARSE_CHAR_PTR) &&
    (aType != SVTK_PARSE_OBJECT_PTR))
  {
    fprintf(fp, "[]");
  }
}

void return_result(FILE* fp)
{
  unsigned int rType = (currentFunction->ReturnType & SVTK_PARSE_UNQUALIFIED_TYPE);

  switch (rType)
  {
    case SVTK_PARSE_FLOAT:
      fprintf(fp, "double ");
      break;
    case SVTK_PARSE_VOID:
      fprintf(fp, "void ");
      break;
    case SVTK_PARSE_CHAR:
      fprintf(fp, "char ");
      break;
    case SVTK_PARSE_DOUBLE:
      fprintf(fp, "double ");
      break;
    case SVTK_PARSE_INT:
    case SVTK_PARSE_SHORT:
    case SVTK_PARSE_LONG:
    case SVTK_PARSE_LONG_LONG:
    case SVTK_PARSE___INT64:
    case SVTK_PARSE_SIGNED_CHAR:
    case SVTK_PARSE_UNSIGNED_CHAR:
    case SVTK_PARSE_UNSIGNED_INT:
    case SVTK_PARSE_UNSIGNED_SHORT:
    case SVTK_PARSE_UNSIGNED_LONG:
    case SVTK_PARSE_UNSIGNED_LONG_LONG:
    case SVTK_PARSE_UNSIGNED___INT64:
    case SVTK_PARSE_UNKNOWN:
      fprintf(fp, "int ");
      break;
    case SVTK_PARSE_BOOL:
      fprintf(fp, "boolean ");
      break;
    case SVTK_PARSE_CHAR_PTR:
    case SVTK_PARSE_STRING:
    case SVTK_PARSE_STRING_REF:
      fprintf(fp, "String ");
      break;
    case SVTK_PARSE_OBJECT_PTR:
      fprintf(fp, "%s ", currentFunction->ReturnClass);
      break;

      /* handle functions returning vectors */
      /* this is done by looking them up in a hint file */
    case SVTK_PARSE_FLOAT_PTR:
    case SVTK_PARSE_DOUBLE_PTR:
      fprintf(fp, "double[] ");
      break;
    case SVTK_PARSE_UNSIGNED_CHAR_PTR:
      fprintf(fp, "byte[] ");
      break;
    case SVTK_PARSE_INT_PTR:
    case SVTK_PARSE_SHORT_PTR:
    case SVTK_PARSE_LONG_PTR:
    case SVTK_PARSE_LONG_LONG_PTR:
    case SVTK_PARSE___INT64_PTR:
    case SVTK_PARSE_SIGNED_CHAR_PTR:
    case SVTK_PARSE_UNSIGNED_INT_PTR:
    case SVTK_PARSE_UNSIGNED_SHORT_PTR:
    case SVTK_PARSE_UNSIGNED_LONG_PTR:
    case SVTK_PARSE_UNSIGNED_LONG_LONG_PTR:
    case SVTK_PARSE_UNSIGNED___INT64_PTR:
      fprintf(fp, "int[]  ");
      break;
    case SVTK_PARSE_BOOL_PTR:
      fprintf(fp, "boolean[]  ");
      break;
  }
}

/* same as return_result except we return a long (the c++ pointer) rather than an object */
void return_result_native(FILE* fp)
{
  unsigned int rType = (currentFunction->ReturnType & SVTK_PARSE_UNQUALIFIED_TYPE);

  switch (rType)
  {
    case SVTK_PARSE_FLOAT:
      fprintf(fp, "double ");
      break;
    case SVTK_PARSE_VOID:
      fprintf(fp, "void ");
      break;
    case SVTK_PARSE_CHAR:
      fprintf(fp, "char ");
      break;
    case SVTK_PARSE_DOUBLE:
      fprintf(fp, "double ");
      break;
    case SVTK_PARSE_INT:
    case SVTK_PARSE_SHORT:
    case SVTK_PARSE_LONG:
    case SVTK_PARSE_LONG_LONG:
    case SVTK_PARSE___INT64:
    case SVTK_PARSE_SIGNED_CHAR:
    case SVTK_PARSE_UNSIGNED_CHAR:
    case SVTK_PARSE_UNSIGNED_INT:
    case SVTK_PARSE_UNSIGNED_SHORT:
    case SVTK_PARSE_UNSIGNED_LONG:
    case SVTK_PARSE_UNSIGNED_LONG_LONG:
    case SVTK_PARSE_UNSIGNED___INT64:
    case SVTK_PARSE_UNKNOWN:
      fprintf(fp, "int ");
      break;
    case SVTK_PARSE_BOOL:
      fprintf(fp, "boolean ");
      break;
    case SVTK_PARSE_CHAR_PTR:
    case SVTK_PARSE_STRING:
    case SVTK_PARSE_STRING_REF:
      fprintf(fp, "String ");
      break;
    case SVTK_PARSE_OBJECT_PTR:
      fprintf(fp, "long ");
      break;

      /* handle functions returning vectors */
      /* this is done by looking them up in a hint file */
    case SVTK_PARSE_FLOAT_PTR:
    case SVTK_PARSE_DOUBLE_PTR:
      fprintf(fp, "double[] ");
      break;
    case SVTK_PARSE_UNSIGNED_CHAR_PTR:
      fprintf(fp, "byte[] ");
      break;
    case SVTK_PARSE_INT_PTR:
    case SVTK_PARSE_SHORT_PTR:
    case SVTK_PARSE_LONG_PTR:
    case SVTK_PARSE_LONG_LONG_PTR:
    case SVTK_PARSE___INT64_PTR:
    case SVTK_PARSE_SIGNED_CHAR_PTR:
    case SVTK_PARSE_UNSIGNED_INT_PTR:
    case SVTK_PARSE_UNSIGNED_SHORT_PTR:
    case SVTK_PARSE_UNSIGNED_LONG_PTR:
    case SVTK_PARSE_UNSIGNED_LONG_LONG_PTR:
    case SVTK_PARSE_UNSIGNED___INT64_PTR:
      fprintf(fp, "int[]  ");
      break;
    case SVTK_PARSE_BOOL_PTR:
      fprintf(fp, "boolean[]  ");
      break;
  }
}

/* Check to see if two types will map to the same Java type,
 * return 1 if type1 should take precedence,
 * return 2 if type2 should take precedence,
 * return 0 if the types do not map to the same type */
static int CheckMatch(unsigned int type1, unsigned int type2, const char* c1, const char* c2)
{
  static unsigned int floatTypes[] = { SVTK_PARSE_DOUBLE, SVTK_PARSE_FLOAT, 0 };

  static unsigned int intTypes[] = { SVTK_PARSE_UNSIGNED_LONG_LONG, SVTK_PARSE_UNSIGNED___INT64,
    SVTK_PARSE_LONG_LONG, SVTK_PARSE___INT64, SVTK_PARSE_UNSIGNED_LONG, SVTK_PARSE_LONG,
    SVTK_PARSE_UNSIGNED_INT, SVTK_PARSE_INT, SVTK_PARSE_UNSIGNED_SHORT, SVTK_PARSE_SHORT,
    SVTK_PARSE_UNSIGNED_CHAR, SVTK_PARSE_SIGNED_CHAR, 0 };

  static unsigned int stringTypes[] = { SVTK_PARSE_CHAR_PTR, SVTK_PARSE_STRING_REF, SVTK_PARSE_STRING,
    0 };

  static unsigned int* numericTypes[] = { floatTypes, intTypes, 0 };

  int i, j;
  int hit1, hit2;

  if ((type1 & SVTK_PARSE_UNQUALIFIED_TYPE) == (type2 & SVTK_PARSE_UNQUALIFIED_TYPE))
  {
    if ((type1 & SVTK_PARSE_BASE_TYPE) == SVTK_PARSE_OBJECT)
    {
      if (strcmp(c1, c2) == 0)
      {
        return 1;
      }
      return 0;
    }
    else
    {
      return 1;
    }
  }

  for (i = 0; numericTypes[i]; i++)
  {
    hit1 = 0;
    hit2 = 0;
    for (j = 0; numericTypes[i][j]; j++)
    {
      if ((type1 & SVTK_PARSE_BASE_TYPE) == numericTypes[i][j])
      {
        hit1 = j + 1;
      }
      if ((type2 & SVTK_PARSE_BASE_TYPE) == numericTypes[i][j])
      {
        hit2 = j + 1;
      }
    }
    if (hit1 && hit2 && (type1 & SVTK_PARSE_INDIRECT) == (type2 & SVTK_PARSE_INDIRECT))
    {
      if (hit1 < hit2)
      {
        return 1;
      }
      else
      {
        return 2;
      }
    }
  }

  hit1 = 0;
  hit2 = 0;
  for (j = 0; stringTypes[j]; j++)
  {
    if ((type1 & SVTK_PARSE_UNQUALIFIED_TYPE) == stringTypes[j])
    {
      hit1 = j + 1;
    }
    if ((type2 & SVTK_PARSE_UNQUALIFIED_TYPE) == stringTypes[j])
    {
      hit2 = j + 1;
    }
  }
  if (hit1 && hit2)
  {
    if (hit1 < hit2)
    {
      return 1;
    }
    else
    {
      return 2;
    }
  }

  return 0;
}

/* have we done one of these yet */
int DoneOne()
{
  int i, j;
  int match;
  FunctionInfo* fi;

  for (i = 0; i < numberOfWrappedFunctions; i++)
  {
    fi = wrappedFunctions[i];

    if ((!strcmp(fi->Name, currentFunction->Name)) &&
      (fi->NumberOfArguments == currentFunction->NumberOfArguments))
    {
      match = 1;
      for (j = 0; j < fi->NumberOfArguments; j++)
      {
        if (!CheckMatch(currentFunction->ArgTypes[j], fi->ArgTypes[j],
              currentFunction->ArgClasses[j], fi->ArgClasses[j]))
        {
          match = 0;
        }
      }
      if (!CheckMatch(currentFunction->ReturnType, fi->ReturnType, currentFunction->ReturnClass,
            fi->ReturnClass))
      {
        match = 0;
      }
      if (match)
        return 1;
    }
  }
  return 0;
}

void HandleDataReader(FILE* fp)
{
  fprintf(fp, "\n  private native void ");
  fprintf(fp, "%s_%i(byte id0[],int id1);\n", currentFunction->Name, numberOfWrappedFunctions);
  fprintf(fp, "\n  public void ");
  fprintf(fp, "%s(byte id0[],int id1)\n", currentFunction->Name);
  fprintf(fp, "    { %s_%i(id0,id1); }\n", currentFunction->Name, numberOfWrappedFunctions);
}

void HandleDataArray(FILE* fp, ClassInfo* data)
{
  const char* type = 0;

  if (!strcmp("svtkCharArray", data->Name))
  {
    type = "char";
  }
  else if (!strcmp("svtkDoubleArray", data->Name))
  {
    type = "double";
  }
  else if (!strcmp("svtkFloatArray", data->Name))
  {
    type = "float";
  }
  else if (!strcmp("svtkIntArray", data->Name))
  {
    type = "int";
  }
  else if (!strcmp("svtkLongArray", data->Name))
  {
    type = "long";
  }
  else if (!strcmp("svtkShortArray", data->Name))
  {
    type = "short";
  }
  else if (!strcmp("svtkUnsignedCharArray", data->Name))
  {
    type = "byte";
  }
  else if (!strcmp("svtkUnsignedIntArray", data->Name))
  {
    type = "int";
  }
  else if (!strcmp("svtkUnsignedLongArray", data->Name))
  {
    type = "long";
  }
  else if (!strcmp("svtkUnsignedShortArray", data->Name))
  {
    type = "short";
  }
  else
  {
    return;
  }

  fprintf(fp, "\n");
  fprintf(fp, "  private native %s[] GetJavaArray_0();\n", type);
  fprintf(fp, "  public %s[] GetJavaArray()\n", type);
  fprintf(fp, "    { return GetJavaArray_0(); }\n");
  fprintf(fp, "\n");
  fprintf(fp, "  private native void SetJavaArray_0(%s[] arr);\n", type);
  fprintf(fp, "  public void SetJavaArray(%s[] arr)\n", type);
  fprintf(fp, "    { SetJavaArray_0(arr); }\n");
}

static int isClassWrapped(const char* classname)
{
  HierarchyEntry* entry;

  if (hierarchyInfo)
  {
    entry = svtkParseHierarchy_FindEntry(hierarchyInfo, classname);

    if (entry == 0 || svtkParseHierarchy_GetProperty(entry, "WRAPEXCLUDE") ||
      !svtkParseHierarchy_IsTypeOf(hierarchyInfo, entry, "svtkObjectBase"))
    {
      return 0;
    }

    /* Only the primary class in the header is wrapped in Java */
    return svtkParseHierarchy_IsPrimary(entry);
  }

  return 1;
}

int checkFunctionSignature(ClassInfo* data)
{
  static unsigned int supported_types[] = { SVTK_PARSE_VOID, SVTK_PARSE_BOOL, SVTK_PARSE_FLOAT,
    SVTK_PARSE_DOUBLE, SVTK_PARSE_CHAR, SVTK_PARSE_UNSIGNED_CHAR, SVTK_PARSE_SIGNED_CHAR, SVTK_PARSE_INT,
    SVTK_PARSE_UNSIGNED_INT, SVTK_PARSE_SHORT, SVTK_PARSE_UNSIGNED_SHORT, SVTK_PARSE_LONG,
    SVTK_PARSE_UNSIGNED_LONG, SVTK_PARSE_LONG_LONG, SVTK_PARSE_UNSIGNED_LONG_LONG, SVTK_PARSE___INT64,
    SVTK_PARSE_UNSIGNED___INT64, SVTK_PARSE_OBJECT, SVTK_PARSE_STRING, SVTK_PARSE_UNKNOWN, 0 };

  int i, j;
  int args_ok = 1;
  unsigned int rType = (currentFunction->ReturnType & SVTK_PARSE_UNQUALIFIED_TYPE);
  unsigned int aType = 0;
  unsigned int baseType = 0;

  /* some functions will not get wrapped no matter what else */
  if (currentFunction->IsOperator || currentFunction->ArrayFailure || currentFunction->IsExcluded ||
    currentFunction->IsDeleted || !currentFunction->IsPublic || !currentFunction->Name)
  {
    return 0;
  }

  /* NewInstance and SafeDownCast can not be wrapped because it is a
     (non-virtual) method which returns a pointer of the same type as
     the current pointer. Since all methods are virtual in Java, this
     looks like polymorphic return type.  */
  if (!strcmp("NewInstance", currentFunction->Name))
  {
    return 0;
  }

  if (!strcmp("SafeDownCast", currentFunction->Name))
  {
    return 0;
  }

  /* The GetInput() in svtkMapper cannot be overridden with a
   * different return type, Java doesn't allow this */
  if (strcmp(data->Name, "svtkMapper") == 0 && strcmp(currentFunction->Name, "GetInput") == 0)
  {
    return 0;
  }

  /* function pointer arguments for callbacks */
  if (currentFunction->NumberOfArguments == 2 &&
    currentFunction->ArgTypes[0] == SVTK_PARSE_FUNCTION &&
    currentFunction->ArgTypes[1] == SVTK_PARSE_VOID_PTR && rType == SVTK_PARSE_VOID)
  {
    return 1;
  }

  /* check to see if we can handle the args */
  for (i = 0; i < currentFunction->NumberOfArguments; i++)
  {
    aType = (currentFunction->ArgTypes[i] & SVTK_PARSE_UNQUALIFIED_TYPE);
    baseType = (aType & SVTK_PARSE_BASE_TYPE);

    for (j = 0; supported_types[j] != 0; j++)
    {
      if (baseType == supported_types[j])
      {
        break;
      }
    }
    if (supported_types[j] == 0)
    {
      args_ok = 0;
    }

    if (baseType == SVTK_PARSE_UNKNOWN)
    {
      const char* qualified_name = 0;
      if ((aType & SVTK_PARSE_INDIRECT) == 0)
      {
        qualified_name = svtkParseHierarchy_QualifiedEnumName(
          hierarchyInfo, data, stringCache, currentFunction->ArgClasses[i]);
      }
      if (qualified_name)
      {
        currentFunction->ArgClasses[i] = qualified_name;
      }
      else
      {
        args_ok = 0;
      }
    }

    if (baseType == SVTK_PARSE_OBJECT)
    {
      if ((aType & SVTK_PARSE_INDIRECT) != SVTK_PARSE_POINTER)
      {
        args_ok = 0;
      }
      else if (!isClassWrapped(currentFunction->ArgClasses[i]))
      {
        args_ok = 0;
      }
    }

    if (aType == SVTK_PARSE_OBJECT)
      args_ok = 0;
    if (((aType & SVTK_PARSE_INDIRECT) != SVTK_PARSE_POINTER) &&
      ((aType & SVTK_PARSE_INDIRECT) != 0) && (aType != SVTK_PARSE_STRING_REF))
      args_ok = 0;
    if (aType == SVTK_PARSE_STRING_PTR)
      args_ok = 0;
    if (aType == SVTK_PARSE_UNSIGNED_CHAR_PTR)
      args_ok = 0;
    if (aType == SVTK_PARSE_UNSIGNED_INT_PTR)
      args_ok = 0;
    if (aType == SVTK_PARSE_UNSIGNED_SHORT_PTR)
      args_ok = 0;
    if (aType == SVTK_PARSE_UNSIGNED_LONG_PTR)
      args_ok = 0;
    if (aType == SVTK_PARSE_UNSIGNED_LONG_LONG_PTR)
      args_ok = 0;
    if (aType == SVTK_PARSE_UNSIGNED___INT64_PTR)
      args_ok = 0;
  }

  baseType = (rType & SVTK_PARSE_BASE_TYPE);

  for (j = 0; supported_types[j] != 0; j++)
  {
    if (baseType == supported_types[j])
    {
      break;
    }
  }
  if (supported_types[j] == 0)
  {
    args_ok = 0;
  }

  if (baseType == SVTK_PARSE_UNKNOWN)
  {
    const char* qualified_name = 0;
    if ((rType & SVTK_PARSE_INDIRECT) == 0)
    {
      qualified_name = svtkParseHierarchy_QualifiedEnumName(
        hierarchyInfo, data, stringCache, currentFunction->ReturnClass);
    }
    if (qualified_name)
    {
      currentFunction->ReturnClass = qualified_name;
    }
    else
    {
      args_ok = 0;
    }
  }

  if (baseType == SVTK_PARSE_OBJECT)
  {
    if ((rType & SVTK_PARSE_INDIRECT) != SVTK_PARSE_POINTER)
    {
      args_ok = 0;
    }
    else if (!isClassWrapped(currentFunction->ReturnClass))
    {
      args_ok = 0;
    }
  }

  if (((rType & SVTK_PARSE_INDIRECT) != SVTK_PARSE_POINTER) && ((rType & SVTK_PARSE_INDIRECT) != 0) &&
    (rType != SVTK_PARSE_STRING_REF))
    args_ok = 0;
  if (rType == SVTK_PARSE_STRING_PTR)
    args_ok = 0;

  /* eliminate unsigned char * and unsigned short * */
  if (rType == SVTK_PARSE_UNSIGNED_INT_PTR)
    args_ok = 0;
  if (rType == SVTK_PARSE_UNSIGNED_SHORT_PTR)
    args_ok = 0;
  if (rType == SVTK_PARSE_UNSIGNED_LONG_PTR)
    args_ok = 0;
  if (rType == SVTK_PARSE_UNSIGNED_LONG_LONG_PTR)
    args_ok = 0;
  if (rType == SVTK_PARSE_UNSIGNED___INT64_PTR)
    args_ok = 0;

  /* make sure we have all the info we need for array arguments in */
  for (i = 0; i < currentFunction->NumberOfArguments; i++)
  {
    aType = (currentFunction->ArgTypes[i] & SVTK_PARSE_UNQUALIFIED_TYPE);

    if (((aType & SVTK_PARSE_INDIRECT) == SVTK_PARSE_POINTER) &&
      (currentFunction->ArgCounts[i] <= 0) && (aType != SVTK_PARSE_OBJECT_PTR) &&
      (aType != SVTK_PARSE_CHAR_PTR))
      args_ok = 0;
  }

  /* if we need a return type hint make sure we have one */
  switch (rType)
  {
    case SVTK_PARSE_FLOAT_PTR:
    case SVTK_PARSE_VOID_PTR:
    case SVTK_PARSE_DOUBLE_PTR:
    case SVTK_PARSE_INT_PTR:
    case SVTK_PARSE_SHORT_PTR:
    case SVTK_PARSE_LONG_PTR:
    case SVTK_PARSE_LONG_LONG_PTR:
    case SVTK_PARSE___INT64_PTR:
    case SVTK_PARSE_SIGNED_CHAR_PTR:
    case SVTK_PARSE_BOOL_PTR:
    case SVTK_PARSE_UNSIGNED_CHAR_PTR:
      args_ok = currentFunction->HaveHint;
      break;
  }

  /* make sure there isn't a Java-specific override */
  if (!strcmp("svtkObject", data->Name))
  {
    /* remove the original svtkCommand observer methods */
    if (!strcmp(currentFunction->Name, "AddObserver") ||
      !strcmp(currentFunction->Name, "GetCommand") ||
      (!strcmp(currentFunction->Name, "RemoveObserver") &&
        (currentFunction->ArgTypes[0] != SVTK_PARSE_UNSIGNED_LONG)) ||
      ((!strcmp(currentFunction->Name, "RemoveObservers") ||
         !strcmp(currentFunction->Name, "HasObserver")) &&
        (((currentFunction->ArgTypes[0] != SVTK_PARSE_UNSIGNED_LONG) &&
           (currentFunction->ArgTypes[0] != (SVTK_PARSE_CHAR_PTR | SVTK_PARSE_CONST))) ||
          (currentFunction->NumberOfArguments > 1))) ||
      (!strcmp(currentFunction->Name, "RemoveAllObservers") &&
        (currentFunction->NumberOfArguments > 0)))
    {
      args_ok = 0;
    }
  }
  else if (!strcmp("svtkObjectBase", data->Name))
  {
    /* remove the special svtkObjectBase methods */
    if (!strcmp(currentFunction->Name, "Print"))
    {
      args_ok = 0;
    }
  }

  /* make sure it isn't a Delete or New function */
  if (!strcmp("Delete", currentFunction->Name) || !strcmp("New", currentFunction->Name))
  {
    args_ok = 0;
  }

  return args_ok;
}

void outputFunction(FILE* fp, ClassInfo* data)
{
  int i;
  unsigned int rType = (currentFunction->ReturnType & SVTK_PARSE_UNQUALIFIED_TYPE);
  int args_ok = checkFunctionSignature(data);

  /* handle DataReader SetBinaryInputString as a special case */
  if (!strcmp("SetBinaryInputString", currentFunction->Name) &&
    (!strcmp("svtkDataReader", data->Name) || !strcmp("svtkStructuredGridReader", data->Name) ||
      !strcmp("svtkRectilinearGridReader", data->Name) ||
      !strcmp("svtkUnstructuredGridReader", data->Name) ||
      !strcmp("svtkStructuredPointsReader", data->Name) || !strcmp("svtkPolyDataReader", data->Name)))
  {
    HandleDataReader(fp);
    wrappedFunctions[numberOfWrappedFunctions] = currentFunction;
    numberOfWrappedFunctions++;
  }

  if (!currentFunction->IsExcluded && currentFunction->IsPublic && args_ok &&
    strcmp(data->Name, currentFunction->Name) && strcmp(data->Name, currentFunction->Name + 1))
  {
    /* make sure we haven't already done one of these */
    if (!DoneOne())
    {
      fprintf(fp, "\n  private native ");
      return_result_native(fp);
      fprintf(fp, "%s_%i(", currentFunction->Name, numberOfWrappedFunctions);

      for (i = 0; i < currentFunction->NumberOfArguments; i++)
      {
        if (i)
        {
          fprintf(fp, ",");
        }
        output_temp(fp, i);

        /* ignore args after function pointer */
        if (currentFunction->ArgTypes[i] == SVTK_PARSE_FUNCTION)
        {
          break;
        }
      }
      fprintf(fp, ");\n");
      fprintf(fp, "  public ");
      return_result(fp);
      fprintf(fp, "%s(", currentFunction->Name);

      for (i = 0; i < currentFunction->NumberOfArguments; i++)
      {
        if (i)
        {
          fprintf(fp, ",");
        }
        output_temp(fp, i);

        /* ignore args after function pointer */
        if (currentFunction->ArgTypes[i] == SVTK_PARSE_FUNCTION)
        {
          break;
        }
      }

      /* if returning object, lookup in global hash */
      if (rType == SVTK_PARSE_OBJECT_PTR)
      {
        fprintf(fp, ") {");
        fprintf(fp, "\n    long temp = %s_%i(", currentFunction->Name, numberOfWrappedFunctions);
        for (i = 0; i < currentFunction->NumberOfArguments; i++)
        {
          if (i)
          {
            fprintf(fp, ",");
          }
          fprintf(fp, "id%i", i);
        }
        fprintf(fp, ");\n");
        fprintf(fp, "\n    if (temp == 0) return null;");
        fprintf(fp, "\n    return (%s)svtkObjectBase.JAVA_OBJECT_MANAGER.getJavaObject(temp);",
          currentFunction->ReturnClass);
        fprintf(fp, "\n}\n");
      }
      else
      {
        /* if not void then need return otherwise none */
        if (rType == SVTK_PARSE_VOID)
        {
          fprintf(fp, ")\n    { %s_%i(", currentFunction->Name, numberOfWrappedFunctions);
        }
        else
        {
          fprintf(fp, ")\n    { return %s_%i(", currentFunction->Name, numberOfWrappedFunctions);
        }
        for (i = 0; i < currentFunction->NumberOfArguments; i++)
        {
          if (i)
          {
            fprintf(fp, ",");
          }
          fprintf(fp, "id%i", i);
        }
        fprintf(fp, "); }\n");
      }

      wrappedFunctions[numberOfWrappedFunctions] = currentFunction;
      numberOfWrappedFunctions++;
    }
  }
}

void WriteDummyClass(FILE* fp, ClassInfo* data, const char* filename)
{
  char* class_name = NULL;
  if (data == NULL)
  {
    char* last_slash = strrchr(filename, '/');
    char* first_dot = strchr(last_slash, '.');
    size_t size = first_dot - last_slash;
    class_name = malloc(size * sizeof(char));
    strncpy(class_name, last_slash + 1, size);
    class_name[size - 1] = '\0';
  }
  else
  {
    class_name = strdup(data->Name);
  }
  fprintf(fp, "package svtk;\n\nclass %s {\n}\n", class_name);
  free(class_name);
}

/* print the parsed structures */
int main(int argc, char* argv[])
{
  OptionInfo* options;
  FileInfo* file_info;
  ClassInfo* data;
  FILE* fp;
  int i;

  /* pre-define a macro to identify the language */
  svtkParse_DefineMacro("__SVTK_WRAP_JAVA__", 0);

  /* get command-line args and parse the header file */
  file_info = svtkParse_Main(argc, argv);

  /* some utility functions require the string cache */
  stringCache = file_info->Strings;

  /* get the command-line options */
  options = svtkParse_GetCommandLineOptions();

  /* get the hierarchy info for accurate typing */
  if (options->HierarchyFileNames)
  {
    hierarchyInfo =
      svtkParseHierarchy_ReadFiles(options->NumberOfHierarchyFileNames, options->HierarchyFileNames);
  }

  /* get the output file */
  fp = fopen(options->OutputFileName, "w");

  if (!fp)
  {
    fprintf(stderr, "Error opening output file %s\n", options->OutputFileName);
    exit(1);
  }

  /* get the main class */
  data = file_info->MainClass;
  if (data == NULL || data->IsExcluded)
  {
    WriteDummyClass(fp, data, options->OutputFileName);
    fclose(fp);
    exit(0);
  }

  if (data->Template)
  {
    WriteDummyClass(fp, data, options->OutputFileName);
    fclose(fp);
    exit(0);
  }

  for (i = 0; i < data->NumberOfSuperClasses; ++i)
  {
    if (strchr(data->SuperClasses[i], '<'))
    {
      WriteDummyClass(fp, data, options->OutputFileName);
      fclose(fp);
      exit(0);
    }
  }

  if (hierarchyInfo)
  {
    if (!svtkWrap_IsTypeOf(hierarchyInfo, data->Name, "svtkObjectBase"))
    {
      WriteDummyClass(fp, data, options->OutputFileName);
      fclose(fp);
      exit(0);
    }

    /* resolve using declarations within the header files */
    svtkWrap_ApplyUsingDeclarations(data, file_info, hierarchyInfo);

    /* expand typedefs */
    svtkWrap_ExpandTypedefs(data, file_info, hierarchyInfo);
  }

  fprintf(fp, "// java wrapper for %s object\n//\n", data->Name);
  fprintf(fp, "\npackage svtk;\n");

  if (strcmp("svtkObjectBase", data->Name))
  {
    fprintf(fp, "import svtk.*;\n");
  }
  fprintf(fp, "\npublic class %s", data->Name);
  if (strcmp("svtkObjectBase", data->Name))
  {
    if (data->NumberOfSuperClasses)
    {
      fprintf(fp, " extends %s", data->SuperClasses[0]);
    }
  }
  fprintf(fp, "\n{\n");

  /* insert function handling code here */
  for (i = 0; i < data->NumberOfFunctions; i++)
  {
    currentFunction = data->Functions[i];
    outputFunction(fp, data);
  }

  HandleDataArray(fp, data);

  if (!data->NumberOfSuperClasses)
  {
    if (strcmp("svtkObjectBase", data->Name) == 0)
    {
      fprintf(fp,
        "\n  public static svtk.svtkJavaMemoryManager JAVA_OBJECT_MANAGER = new "
        "svtk.svtkJavaMemoryManagerImpl();");
    }
    if (!data->IsAbstract)
    {
      fprintf(fp, "\n  public %s() {", data->Name);
      fprintf(fp, "\n    this.svtkId = this.SVTKInit();");
      fprintf(fp, "\n    svtkObjectBase.JAVA_OBJECT_MANAGER.registerJavaObject(this.svtkId, this);");
      fprintf(fp, "\n}\n");
    }
    else
    {
      fprintf(fp, "\n  public %s() { super(); }\n", data->Name);
    }
    fprintf(fp, "\n  public %s(long id) {", data->Name);
    fprintf(fp, "\n    super();");
    fprintf(fp, "\n    this.svtkId = id;");
    fprintf(fp, "\n    this.SVTKRegister();");
    fprintf(fp, "\n    svtkObjectBase.JAVA_OBJECT_MANAGER.registerJavaObject(this.svtkId, this);");
    fprintf(fp, "\n}\n");
    fprintf(fp, "\n  protected long svtkId;\n");
    fprintf(fp, "\n  public long GetSVTKId() { return this.svtkId; }");

    /* if we are a base class and have a delete method */
    if (data->HasDelete)
    {
      fprintf(fp, "\n  public static native void SVTKDeleteReference(long id);");
      fprintf(fp, "\n  public static native String SVTKGetClassNameFromReference(long id);");
      fprintf(fp, "\n  protected native void SVTKDelete();");
      fprintf(fp, "\n  protected native void SVTKRegister();");
      fprintf(fp, "\n  public void Delete() {");
      fprintf(fp, "\n    svtkObjectBase.JAVA_OBJECT_MANAGER.unRegisterJavaObject(this.svtkId);");
      fprintf(fp, "\n    this.svtkId = 0;");
      fprintf(fp, "\n  }");
    }
  }
  else
  {
    fprintf(fp, "\n  public %s() { super(); }\n", data->Name);
    fprintf(fp, "\n  public %s(long id) { super(id); }\n", data->Name);
  }

  if (!data->IsAbstract)
  {
    fprintf(fp, "  public native long   SVTKInit();\n");
  }

  /* fprintf(fp,"  protected native void   SVTKCastInit();\n"); */

  if (!strcmp("svtkObjectBase", data->Name))
  {
    /* Add the Print method to svtkObjectBase. */
    fprintf(fp, "  public native String Print();\n");
    /* Add the default toString from java object */
    fprintf(fp, "  public String toString() { return Print(); }\n");
  }

  if (!strcmp("svtkObject", data->Name))
  {
    fprintf(fp, "  public native int AddObserver(String id0, Object id1, String id2);\n");
  }
  fprintf(fp, "\n}\n");
  fclose(fp);
  {
    size_t cc;
    size_t len;
    char* dir;
    char* fname;
    /*const */ char javaDone[] = "SVTKJavaWrapped";
    FILE* tfp;
    fname = options->OutputFileName;
    dir = (char*)malloc(strlen(fname) + strlen(javaDone) + 2);
    sprintf(dir, "%s", fname);
    len = strlen(dir);
    for (cc = len - 1; cc > 0; cc--)
    {
      if (dir[cc] == '/' || dir[cc] == '\\')
      {
        dir[cc + 1] = 0;
        break;
      }
    }
    strcat(dir, javaDone);
    tfp = fopen(dir, "w");
    if (tfp)
    {
      fprintf(tfp, "File: %s\n", fname);
      fclose(tfp);
    }
    free(dir);
  }

  svtkParse_Free(file_info);

  return 0;
}
