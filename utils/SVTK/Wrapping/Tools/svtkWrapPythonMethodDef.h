/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkWrapPythonMethodDef.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#ifndef svtkWrapPythonMethodDef_h
#define svtkWrapPythonMethodDef_h

#include "svtkParse.h"
#include "svtkParseData.h"
#include "svtkParseHierarchy.h"

/* check whether a method is wrappable */
int svtkWrapPython_MethodCheck(ClassInfo* data, FunctionInfo* currentFunction, HierarchyInfo* hinfo);

/* print out all methods and the method table */
void svtkWrapPython_GenerateMethods(FILE* fp, const char* classname, ClassInfo* data,
  FileInfo* finfo, HierarchyInfo* hinfo, int is_svtkobject, int do_constructors);

#endif /* svtkWrapPythonMethodDef_h */
/* SVTK-HeaderTest-Exclude: svtkWrapPythonMethodDef.h */
