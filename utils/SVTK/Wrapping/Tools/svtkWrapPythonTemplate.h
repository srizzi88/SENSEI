/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkWrapPythonTemplate.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#ifndef svtkWrapPythonTemplate_h
#define svtkWrapPythonTemplate_h

#include "svtkParse.h"
#include "svtkParseData.h"
#include "svtkParseHierarchy.h"

/* if name has template args, convert to pythonic dict format */
size_t svtkWrapPython_PyTemplateName(const char* name, char* pname);

/* wrap a templated class */
int svtkWrapPython_WrapTemplatedClass(
  FILE* fp, ClassInfo* data, FileInfo* file_info, HierarchyInfo* hinfo);

#endif /* svtkWrapPythonTemplate_h */
/* SVTK-HeaderTest-Exclude: svtkWrapPythonTemplate.h */
