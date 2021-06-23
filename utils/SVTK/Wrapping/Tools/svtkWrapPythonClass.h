/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkWrapPythonClass.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#ifndef svtkWrapPythonClass_h
#define svtkWrapPythonClass_h

#include "svtkParse.h"
#include "svtkParseData.h"
#include "svtkParseHierarchy.h"

/* Wrap one class, returns zero if not wrappable */
int svtkWrapPython_WrapOneClass(FILE* fp, const char* module, const char* classname, ClassInfo* data,
  FileInfo* file_info, HierarchyInfo* hinfo, int is_svtkobject);

/* get the true superclass */
const char* svtkWrapPython_GetSuperClass(ClassInfo* data, HierarchyInfo* hinfo);

/* check whether the superclass of the specified class is wrapped,
   the module for the superclass is returned and is_external is
   set if the module is different from ours */
const char* svtkWrapPython_HasWrappedSuperClass(
  HierarchyInfo* hinfo, const char* classname, int* is_external);

/* generate the class docstring and write it to "fp" */
void svtkWrapPython_ClassDoc(
  FILE* fp, FileInfo* file_info, ClassInfo* data, HierarchyInfo* hinfo, int is_svtkobject);

#endif /* svtkWrapPythonClass_h */
/* SVTK-HeaderTest-Exclude: svtkWrapPythonClass.h */
