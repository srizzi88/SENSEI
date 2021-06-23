/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkWrapPythonType.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#ifndef svtkWrapPythonType_h
#define svtkWrapPythonType_h

#include "svtkParse.h"
#include "svtkParseData.h"
#include "svtkParseHierarchy.h"

/* check whether a non-svtkObjectBase class is wrappable */
int svtkWrapPython_IsSpecialTypeWrappable(ClassInfo* data);

/* write out a python type object */
void svtkWrapPython_GenerateSpecialType(FILE* fp, const char* module, const char* classname,
  ClassInfo* data, FileInfo* finfo, HierarchyInfo* hinfo);

#endif /* svtkWrapPythonType_h */
/* SVTK-HeaderTest-Exclude: svtkWrapPythonType.h */
