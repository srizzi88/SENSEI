/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkParse.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

/*
  This is the header file for svtkParse.tab.c, which is generated
  from svtkParse.y with the "yacc" compiler-compiler.
*/

#ifndef svtkParse_h
#define svtkParse_h

#include "svtkParseData.h"
#include "svtkParseType.h"
#include "svtkWrappingToolsModule.h"
#include <stdio.h>

#ifdef __cplusplus
extern "C"
{
#endif

  /**
   * Define a preprocessor macro. Function macros are not supported.
   */
  SVTKWRAPPINGTOOLS_EXPORT
  void svtkParse_DefineMacro(const char* name, const char* definition);

  /**
   * Undefine a preprocessor macro.
   */
  SVTKWRAPPINGTOOLS_EXPORT
  void svtkParse_UndefineMacro(const char* name);

  /**
   * Do not pre-define any macros related to the system or platform.
   */
  SVTKWRAPPINGTOOLS_EXPORT
  void svtkParse_UndefinePlatformMacros(void);

  /**
   * Read macros from the provided header file.
   */
  SVTKWRAPPINGTOOLS_EXPORT
  void svtkParse_IncludeMacros(const char* filename);

  /**
   * Dump macros to the specified file (stdout if NULL).
   */
  SVTKWRAPPINGTOOLS_EXPORT
  void svtkParse_DumpMacros(const char* filename);

  /**
   * Add an include directory, for use with the "-I" option.
   */
  SVTKWRAPPINGTOOLS_EXPORT
  void svtkParse_IncludeDirectory(const char* dirname);

  /**
   * Return the full path to a header file.
   */
  SVTKWRAPPINGTOOLS_EXPORT
  const char* svtkParse_FindIncludeFile(const char* filename);

  /**
   * Set the command name, for error reporting and diagnostics.
   */
  SVTKWRAPPINGTOOLS_EXPORT
  void svtkParse_SetCommandName(const char* name);

  /**
   * Parse a header file and return a FileInfo struct
   */
  SVTKWRAPPINGTOOLS_EXPORT
  FileInfo* svtkParse_ParseFile(const char* filename, FILE* ifile, FILE* errfile);

  /**
   * Read a hints file and update the FileInfo
   */
  SVTKWRAPPINGTOOLS_EXPORT
  int svtkParse_ReadHints(FileInfo* data, FILE* hfile, FILE* errfile);

  /**
   * Free the FileInfo struct returned by svtkParse_ParseFile()
   */
  SVTKWRAPPINGTOOLS_EXPORT
  void svtkParse_Free(FileInfo* data);

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif
/* SVTK-HeaderTest-Exclude: svtkParse.h */
