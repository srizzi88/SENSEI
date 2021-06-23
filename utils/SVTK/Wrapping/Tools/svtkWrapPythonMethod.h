/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkWrapPythonMethod.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#ifndef svtkWrapPythonMethod_h
#define svtkWrapPythonMethod_h

#include "svtkParse.h"
#include "svtkParseData.h"
#include "svtkParseHierarchy.h"

/* print out the code for one method, including all of its overloads */
void svtkWrapPython_GenerateOneMethod(FILE* fp, const char* classname, ClassInfo* data,
  HierarchyInfo* hinfo, FunctionInfo* wrappedFunctions[], int numberOfWrappedFunctions, int fnum,
  int is_svtkobject, int do_constructors);

/* declare all variables needed by the wrapper method */
void svtkWrapPython_DeclareVariables(FILE* fp, ClassInfo* data, FunctionInfo* theFunc);

/* Write the code to convert an argument with svtkPythonArgs */
void svtkWrapPython_GetSingleArgument(
  FILE* fp, ClassInfo* data, int i, ValueInfo* arg, int call_static);

/* print the code to build python return value from a method */
void svtkWrapPython_ReturnValue(FILE* fp, ClassInfo* data, ValueInfo* val, int static_call);

#endif /* svtkWrapPythonMethod_h */
/* SVTK-HeaderTest-Exclude: svtkWrapPythonMethod.h */
