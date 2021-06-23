/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkWrapPythonOverload.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#ifndef svtkWrapPythonOverload_h
#define svtkWrapPythonOverload_h

#include "svtkParse.h"
#include "svtkParseData.h"
#include "svtkParseHierarchy.h"

/* output the method table for all overloads of a particular method */
void svtkWrapPython_OverloadMethodDef(FILE* fp, const char* classname, ClassInfo* data,
  int* overloadMap, FunctionInfo** wrappedFunctions, int numberOfWrappedFunctions, int fnum,
  int numberOfOccurrences, int all_legacy);

/* a master method to choose which overload to call */
void svtkWrapPython_OverloadMasterMethod(FILE* fp, const char* classname, int* overloadMap,
  int maxArgs, FunctionInfo** wrappedFunctions, int numberOfWrappedFunctions, int fnum,
  int is_svtkobject, int all_legacy);

/* generate an int array that maps arg counts to overloads */
int* svtkWrapPython_ArgCountToOverloadMap(FunctionInfo** wrappedFunctions,
  int numberOfWrappedFunctions, int fnum, int is_svtkobject, int* nmax, int* overlap);

#endif /* svtkWrapPythonOverload_h */
/* SVTK-HeaderTest-Exclude: svtkWrapPythonOverload.h */
