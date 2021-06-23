/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkWrapPythonOverload.c

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/*
  When wrapping overloaded C++ methods, it is necessary to provide
  hints so that Python can choose which overload to call (see
  svtkPythonOverload.cxx for the code that is used to do this).

  Where possible, overloads are resolved based on the number of
  arguments that are passed.  When this isn't possible, the overloads
  must be resolved based on argument types.  So, for each overload,
  we store the parameter types as a string.

  The "parameter type" string can start with one of the following:

    - (hyphen) marks a method as an explicit constructor
    @ placeholder for "self" in a method (i.e. method is not static)

  For each parameter, one of the following codes is used:

    q bool
    c char
    b signed char
    B unsigned char
    h signed short
    H unsigned short
    i int
    I unsigned int
    l long
    L unsigned long
    k long long
    K unsigned long long
    f float
    d double
    v void *
    z char *
    s string
    u unicode
    F callable object
    E enum type
    O python object
    Q Qt object
    V SVTK object
    W SVTK special type
    P Pointer to numeric type
    A Multi-dimensional array of numeric type

    | marks the end of required parameters, following parameters are optional

  If the parameter is E, O, Q, V, W, then a type name must follow the type
  codes. The type name must be preceded by '*' if the type is a non-const
  reference or a pointer.  For example,

    func(svtkArray *, svtkVariant &, int) -> "VWi *svtkArray &svtkVariant"

  If the parameter is P, then the type of the array or pointer must
  follow the type codes.  For example,

    func(int *p, double a[10]) -> "PP *i *d"

  If the parameter is A, then both the type and all dimensions after the
  first dimension must be provided:

    func(double a[3][4]) -> "A *d[4]"

*/

#include "svtkWrapPythonOverload.h"
#include "svtkWrapPythonMethod.h"
#include "svtkWrapPythonTemplate.h"

#include "svtkWrap.h"
#include "svtkWrapText.h"

#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* -------------------------------------------------------------------- */
/* prototypes for utility methods */

/* create a parameter format char for the give type */
static char svtkWrapPython_FormatChar(unsigned int argtype);

/* create a string for checking arguments against available signatures */
static char* svtkWrapPython_ArgCheckString(ClassInfo* data, FunctionInfo* currentFunction);

/* -------------------------------------------------------------------- */
/* Get the python format char for the give type, after retrieving the
 * base type from the type */
static char svtkWrapPython_FormatChar(unsigned int argtype)
{
  char typeChar = 'O';

  switch ((argtype & SVTK_PARSE_BASE_TYPE))
  {
    case SVTK_PARSE_FLOAT:
      typeChar = 'f';
      break;
    case SVTK_PARSE_DOUBLE:
      typeChar = 'd';
      break;
    case SVTK_PARSE_UNSIGNED_INT:
      typeChar = 'I';
      break;
    case SVTK_PARSE_INT:
      typeChar = 'i';
      break;
    case SVTK_PARSE_UNSIGNED_SHORT:
      typeChar = 'H';
      break;
    case SVTK_PARSE_SHORT:
      typeChar = 'h';
      break;
    case SVTK_PARSE_UNSIGNED_LONG:
      typeChar = 'L';
      break;
    case SVTK_PARSE_LONG:
      typeChar = 'l';
      break;
    case SVTK_PARSE_SIZE_T:
    case SVTK_PARSE_UNSIGNED_LONG_LONG:
    case SVTK_PARSE_UNSIGNED___INT64:
      typeChar = 'K';
      break;
    case SVTK_PARSE_SSIZE_T:
    case SVTK_PARSE_LONG_LONG:
    case SVTK_PARSE___INT64:
      typeChar = 'k';
      break;
    case SVTK_PARSE_SIGNED_CHAR:
      typeChar = 'b';
      break;
    case SVTK_PARSE_CHAR:
      typeChar = 'c';
      break;
    case SVTK_PARSE_UNSIGNED_CHAR:
      typeChar = 'B';
      break;
    case SVTK_PARSE_VOID:
      typeChar = 'v';
      break;
    case SVTK_PARSE_BOOL:
      typeChar = 'q';
      break;
    case SVTK_PARSE_STRING:
      typeChar = 's';
      break;
    case SVTK_PARSE_UNICODE_STRING:
      typeChar = 'u';
      break;
  }

  return typeChar;
}

/* -------------------------------------------------------------------- */
/* Create a string to describe the signature of a method. */

static char* svtkWrapPython_ArgCheckString(ClassInfo* data, FunctionInfo* currentFunction)
{
  static char result[2048]; /* max literal string length */
  char classname[1024];
  size_t currPos = 0;
  size_t endPos;
  ValueInfo* arg;
  unsigned int argtype;
  int i, j, k;
  int totalArgs, requiredArgs;
  char c = '\0';

  totalArgs = svtkWrap_CountWrappedParameters(currentFunction);
  requiredArgs = svtkWrap_CountRequiredArguments(currentFunction);

  if (currentFunction->IsExplicit)
  {
    /* used to mark constructors as 'explicit' */
    result[currPos++] = '-';
  }

  /* placeholder for 'self' in method calls */
  if (!currentFunction->IsStatic)
  {
    result[currPos++] = '@';
  }

  /* position for insertion after format chars */
  endPos = currPos + totalArgs;
  if (totalArgs > requiredArgs)
  {
    /* add one for the "|" that marks the end of required args */
    endPos++;
  }

  /* create a format character for each argument */
  for (i = 0; i < totalArgs; i++)
  {
    arg = currentFunction->Parameters[i];
    argtype = (arg->Type & SVTK_PARSE_UNQUALIFIED_TYPE);

    if (i == requiredArgs)
    {
      /* make all following arguments optional */
      result[currPos++] = '|';
    }

    /* will store the classname for objects */
    classname[0] = '\0';

    if (svtkWrap_IsEnumMember(data, arg))
    {
      c = 'E';
      sprintf(classname, "%.200s.%.200s", data->Name, arg->Class);
    }
    else if (arg->IsEnum)
    {
      c = 'E';
      svtkWrapText_PythonName(arg->Class, classname);
    }
    else if (svtkWrap_IsPythonObject(arg))
    {
      c = 'O';
      svtkWrapText_PythonName(arg->Class, classname);
    }
    else if (svtkWrap_IsSVTKObject(arg))
    {
      c = 'V';
      svtkWrapText_PythonName(arg->Class, classname);
    }
    else if (svtkWrap_IsSpecialObject(arg))
    {
      c = 'W';
      svtkWrapText_PythonName(arg->Class, classname);
    }
    else if (svtkWrap_IsFunction(arg))
    {
      c = 'F';
    }
    else if (svtkWrap_IsVoidPointer(arg))
    {
      c = 'v';
    }
    else if (svtkWrap_IsString(arg))
    {
      c = 's';
      if ((argtype & SVTK_PARSE_BASE_TYPE) == SVTK_PARSE_UNICODE_STRING)
      {
        c = 'u';
      }
    }
    else if (svtkWrap_IsCharPointer(arg))
    {
      c = 'z';
    }
    else if (svtkWrap_IsNumeric(arg) && svtkWrap_IsScalar(arg))
    {
      c = svtkWrapPython_FormatChar(argtype);
    }
    else if (svtkWrap_IsArray(arg) || svtkWrap_IsPODPointer(arg))
    {
      c = 'P';
      result[endPos++] = ' ';
      result[endPos++] = '*';
      result[endPos++] = svtkWrapPython_FormatChar(argtype);
    }
    else if (svtkWrap_IsNArray(arg))
    {
      c = 'A';
      result[endPos++] = ' ';
      result[endPos++] = '*';
      result[endPos++] = svtkWrapPython_FormatChar(argtype);
      if (svtkWrap_IsNArray(arg))
      {
        for (j = 1; j < arg->NumberOfDimensions; j++)
        {
          result[endPos++] = '[';
          for (k = 0; arg->Dimensions[j][k]; k++)
          {
            result[endPos++] = arg->Dimensions[j][k];
          }
          result[endPos++] = ']';
        }
      }
    }

    /* add the format char to the string */
    result[currPos++] = c;
    if (classname[0] != '\0')
    {
      result[endPos++] = ' ';
      if ((argtype == SVTK_PARSE_OBJECT_REF || argtype == SVTK_PARSE_QOBJECT_REF ||
            argtype == SVTK_PARSE_UNKNOWN_REF) &&
        (arg->Type & SVTK_PARSE_CONST) == 0)
      {
        result[endPos++] = '&';
      }
      else if (argtype == SVTK_PARSE_OBJECT_PTR || argtype == SVTK_PARSE_UNKNOWN_PTR ||
        argtype == SVTK_PARSE_QOBJECT_PTR)
      {
        result[endPos++] = '*';
      }
      strcpy(&result[endPos], classname);
      endPos += strlen(classname);
    }
  }

  result[endPos] = '\0';
  return result;
}

/* -------------------------------------------------------------------- */
/* Generate an int array that maps arg counts to overloads.
 * Each element in the array will either contain the index of the
 * overload that it maps to, or -1 if it maps to multiple overloads,
 * or zero if it does not map to any.  The length of the array is
 * returned in "nmax". The value of "overlap" is set to 1 if there
 * are some arg counts that map to more than one method. */

int* svtkWrapPython_ArgCountToOverloadMap(FunctionInfo** wrappedFunctions,
  int numberOfWrappedFunctions, int fnum, int is_svtkobject, int* nmax, int* overlap)
{
  static int overloadMap[512];
  int totalArgs, requiredArgs;
  int occ, occCounter;
  FunctionInfo* theOccurrence;
  FunctionInfo* theFunc;
  int mixed_static, any_static;
  int i;

  *nmax = 0;
  *overlap = 0;

  theFunc = wrappedFunctions[fnum];

  any_static = 0;
  mixed_static = 0;
  for (i = fnum; i < numberOfWrappedFunctions; i++)
  {
    if (wrappedFunctions[i]->Name && strcmp(wrappedFunctions[i]->Name, theFunc->Name) == 0)
    {
      if (wrappedFunctions[i]->IsStatic)
      {
        any_static = 1;
      }
      else if (any_static)
      {
        mixed_static = 1;
      }
    }
  }

  for (i = 0; i < 100; i++)
  {
    overloadMap[i] = 0;
  }

  occCounter = 0;
  for (occ = fnum; occ < numberOfWrappedFunctions; occ++)
  {
    theOccurrence = wrappedFunctions[occ];

    if (theOccurrence->Name == 0 || strcmp(theOccurrence->Name, theFunc->Name) != 0)
    {
      continue;
    }

    occCounter++;

    totalArgs = svtkWrap_CountWrappedParameters(theOccurrence);
    requiredArgs = svtkWrap_CountRequiredArguments(theOccurrence);

    /* svtkobject calls might have an extra "self" arg in front */
    if (mixed_static && is_svtkobject && !theOccurrence->IsStatic)
    {
      totalArgs++;
    }

    if (totalArgs > *nmax)
    {
      *nmax = totalArgs;
    }

    for (i = requiredArgs; i <= totalArgs && i < 100; i++)
    {
      if (overloadMap[i] == 0)
      {
        overloadMap[i] = occCounter;
      }
      else
      {
        overloadMap[i] = -1;
        *overlap = 1;
      }
    }
  }

  return overloadMap;
}

/* -------------------------------------------------------------------- */
/* output the method table for all overloads of a particular method,
 * this is also used to write out all constructors for the class */

void svtkWrapPython_OverloadMethodDef(FILE* fp, const char* classname, ClassInfo* data,
  int* overloadMap, FunctionInfo** wrappedFunctions, int numberOfWrappedFunctions, int fnum,
  int numberOfOccurrences, int all_legacy)
{
  char occSuffix[16];
  int occ, occCounter;
  FunctionInfo* theOccurrence;
  FunctionInfo* theFunc;
  int totalArgs, requiredArgs;
  int i;
  int putInTable;

  theFunc = wrappedFunctions[fnum];

  if (all_legacy)
  {
    fprintf(fp, "#if !defined(SVTK_LEGACY_REMOVE)\n");
  }

  fprintf(fp, "static PyMethodDef Py%s_%s_Methods[] = {\n", classname, theFunc->Name);

  occCounter = 0;
  for (occ = fnum; occ < numberOfWrappedFunctions; occ++)
  {
    theOccurrence = wrappedFunctions[occ];

    if (theOccurrence->Name == 0 || strcmp(theOccurrence->Name, theFunc->Name) != 0)
    {
      continue;
    }

    occCounter++;

    totalArgs = svtkWrap_CountWrappedParameters(theOccurrence);
    requiredArgs = svtkWrap_CountRequiredArguments(theOccurrence);

    putInTable = 0;

    /* all conversion constructors must go into the table */
    if (svtkWrap_IsConstructor(data, theOccurrence) && requiredArgs <= 1 && totalArgs >= 1 &&
      !theOccurrence->IsExplicit)
    {
      putInTable = 1;
    }

    /* all methods that overlap with others must go in the table */
    for (i = requiredArgs; i <= totalArgs; i++)
    {
      if (overloadMap[i] == -1)
      {
        putInTable = 1;
      }
    }

    if (!putInTable)
    {
      continue;
    }

    if (theOccurrence->IsLegacy && !all_legacy)
    {
      fprintf(fp, "#if !defined(SVTK_LEGACY_REMOVE)\n");
    }

    /* method suffix to distinguish between signatures */
    occSuffix[0] = '\0';
    if (numberOfOccurrences > 1)
    {
      sprintf(occSuffix, "_s%d", occCounter);
    }

    fprintf(fp,
      "  {nullptr, Py%s_%s%s, METH_VARARGS%s,\n"
      "   \"%s\"},\n",
      classname, theOccurrence->Name, occSuffix, theOccurrence->IsStatic ? " | METH_STATIC" : "",
      svtkWrapPython_ArgCheckString(data, theOccurrence));

    if (theOccurrence->IsLegacy && !all_legacy)
    {
      fprintf(fp, "#endif\n");
    }
  }

  fprintf(fp,
    "  {nullptr, nullptr, 0, nullptr}\n"
    "};\n");

  if (all_legacy)
  {
    fprintf(fp, "#endif\n");
  }

  fprintf(fp, "\n");
}

/* -------------------------------------------------------------------- */
/* make a method that will choose which overload to call */

void svtkWrapPython_OverloadMasterMethod(FILE* fp, const char* classname, int* overloadMap,
  int maxArgs, FunctionInfo** wrappedFunctions, int numberOfWrappedFunctions, int fnum,
  int is_svtkobject, int all_legacy)
{
  FunctionInfo* currentFunction;
  FunctionInfo* theOccurrence;
  int overlap = 0;
  int occ, occCounter;
  int i;
  int foundOne;
  int any_static = 0;

  currentFunction = wrappedFunctions[fnum];

  for (i = fnum; i < numberOfWrappedFunctions; i++)
  {
    if (wrappedFunctions[i]->Name && strcmp(wrappedFunctions[i]->Name, currentFunction->Name) == 0)
    {
      if (wrappedFunctions[i]->IsStatic)
      {
        any_static = 1;
      }
    }
  }

  for (i = 0; i <= maxArgs; i++)
  {
    if (overloadMap[i] == -1)
    {
      overlap = 1;
    }
  }

  if (all_legacy)
  {
    fprintf(fp, "#if !defined(SVTK_LEGACY_REMOVE)\n");
  }

  fprintf(fp,
    "static PyObject *\n"
    "Py%s_%s(PyObject *self, PyObject *args)\n"
    "{\n",
    classname, currentFunction->Name);

  if (overlap)
  {
    fprintf(fp, "  PyMethodDef *methods = Py%s_%s_Methods;\n", classname, currentFunction->Name);
  }

  fprintf(fp,
    "  int nargs = svtkPythonArgs::GetArgCount(%sargs);\n"
    "\n",
    ((is_svtkobject && !any_static) ? "self, " : ""));

  fprintf(fp,
    "  switch(nargs)\n"
    "  {\n");

  /* find all occurrences of this method */
  occCounter = 0;
  for (occ = fnum; occ < numberOfWrappedFunctions; occ++)
  {
    theOccurrence = wrappedFunctions[occ];

    /* is it the same name */
    if (theOccurrence->Name && strcmp(currentFunction->Name, theOccurrence->Name) == 0)
    {
      occCounter++;

      foundOne = 0;
      for (i = 0; i <= maxArgs; i++)
      {
        if (overloadMap[i] == occCounter)
        {
          if (!foundOne && theOccurrence->IsLegacy && !all_legacy)
          {
            fprintf(fp, "#if !defined(SVTK_LEGACY_REMOVE)\n");
          }
          fprintf(fp, "    case %d:\n", i);
          foundOne = 1;
        }
      }
      if (foundOne)
      {
        fprintf(fp, "      return Py%s_%s_s%d(self, args);\n", classname, currentFunction->Name,
          occCounter);
        if (theOccurrence->IsLegacy && !all_legacy)
        {
          fprintf(fp, "#endif\n");
        }
      }
    }
  }

  if (overlap)
  {
    for (i = 0; i <= maxArgs; i++)
    {
      if (overloadMap[i] == -1)
      {
        fprintf(fp, "    case %d:\n", i);
      }
    }
    fprintf(fp, "      return svtkPythonOverload::CallMethod(methods, self, args);\n");
  }

  fprintf(fp,
    "  }\n"
    "\n");

  fprintf(fp, "  svtkPythonArgs::ArgCountError(nargs, \"%.200s\");\n", currentFunction->Name);

  fprintf(fp,
    "  return nullptr;\n"
    "}\n"
    "\n");

  if (all_legacy)
  {
    fprintf(fp, "#endif\n");
  }

  fprintf(fp, "\n");
}
