/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkWrapPythonConstant.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#ifndef svtkWrapPythonConstant_h
#define svtkWrapPythonConstant_h

#include "svtkParse.h"
#include "svtkParseData.h"
#include "svtkParseHierarchy.h"

/* generate code that adds a constant value to a python dict */
void svtkWrapPython_AddConstant(FILE* fp, const char* indent, const char* dictvar,
  const char* objvar, const char* scope, ValueInfo* val);

/* generate code that adds all public constants in a namespace */
void svtkWrapPython_AddPublicConstants(
  FILE* fp, const char* indent, const char* dictvar, const char* objvar, NamespaceInfo* data);

#endif /* svtkWrapPythonConstant_h */
/* SVTK-HeaderTest-Exclude: svtkWrapPythonConstant.h */
