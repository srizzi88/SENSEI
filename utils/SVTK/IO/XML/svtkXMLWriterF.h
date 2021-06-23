/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkXMLWriterF.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#ifndef svtkXMLWriterF_h
#define svtkXMLWriterF_h
/*
 * svtkXMLWriterF.h helps fortran programs call the C interface for
 * writing SVTK XML files.  A program can use this by writing one
 * svtkXMLWriterF.c file that includes this header.  DO NOT INCLUDE
 * THIS HEADER ELSEWHERE.  The fortran program then compiles
 * svtkXMLWriterF.c using a C compiler and links to the resulting
 * object file.
 */

#if defined(__cplusplus)
#error "This should be included only by a .c file."
#endif

/* Calls will be forwarded to the C interface.  */
#include "svtkXMLWriterC.h"

#include <stdio.h>  /* fprintf */
#include <stdlib.h> /* malloc, free */
#include <string.h> /* memcpy */

/* Define a static-storage default-zero-initialized table to store
   writer objects for the fortran program.  */
#define SVTK_XMLWRITERF_MAX 256
static svtkXMLWriterC* svtkXMLWriterF_Table[SVTK_XMLWRITERF_MAX + 1];

/* Fortran compilers expect certain symbol names for their calls to C
   code.  These macros help build the C symbols so that the fortran
   program can link to them properly.  The definitions here are
   reasonable defaults but the source file that includes this can
   define them appropriately for a particular compiler and override
   these.  */
#if !defined(SVTK_FORTRAN_NAME)
#define SVTK_FORTRAN_NAME(name, NAME) name##__
#endif
#if !defined(SVTK_FORTRAN_ARG_STRING_POINTER)
#define SVTK_FORTRAN_ARG_STRING_POINTER(name) const char* name##_ptr_arg
#endif
#if !defined(SVTK_FORTRAN_ARG_STRING_LENGTH)
#define SVTK_FORTRAN_ARG_STRING_LENGTH(name) , const long int name##_len_arg
#endif
#if !defined(SVTK_FORTRAN_REF_STRING_POINTER)
#define SVTK_FORTRAN_REF_STRING_POINTER(name) name##_ptr_arg
#endif
#if !defined(SVTK_FORTRAN_REF_STRING_LENGTH)
#define SVTK_FORTRAN_REF_STRING_LENGTH(name) ((int)name##_len_arg)
#endif

/*--------------------------------------------------------------------------*/
/* svtkXMLWriterF_New */
void SVTK_FORTRAN_NAME(svtkxmlwriterf_new, SVTKXMLWRITERF_NEW)(int* self)
{
  int i;

  /* Initialize result to failure.  */
  *self = 0;

  /* Search for a table entry to use for this object.  */
  for (i = 1; i <= SVTK_XMLWRITERF_MAX; ++i)
  {
    if (!svtkXMLWriterF_Table[i])
    {
      svtkXMLWriterF_Table[i] = svtkXMLWriterC_New();
      if (svtkXMLWriterF_Table[i])
      {
        *self = i;
      }
      return;
    }
  }
}

/*--------------------------------------------------------------------------*/
/* svtkXMLWriterF_Delete */
void SVTK_FORTRAN_NAME(svtkxmlwriterf_delete, SVTKXMLWRITERF_DELETE)(int* self)
{
  /* Check if the writer object exists.  */
  if (*self > 0 && *self <= SVTK_XMLWRITERF_MAX && svtkXMLWriterF_Table[*self])
  {
    /* Delete this writer object.  */
    svtkXMLWriterC_Delete(svtkXMLWriterF_Table[*self]);

    /* Erase the table entry.  */
    svtkXMLWriterF_Table[*self] = 0;
  }
  else
  {
    fprintf(stderr, "svtkXMLWriterF_Delete called with invalid id %d.\n", *self);
  }

  /* The writer object no longer exists.  Destroy the id.  */
  *self = 0;
}

/*--------------------------------------------------------------------------*/
/* svtkXMLWriterF_SetDataModeType */
void SVTK_FORTRAN_NAME(svtkxmlwriterf_setdatamodetype, SVTKXMLWRITERF_SETDATAMODETYPE)(
  const int* self, const int* objType)
{
  if (*self > 0 && *self <= SVTK_XMLWRITERF_MAX && svtkXMLWriterF_Table[*self])
  {
    svtkXMLWriterC_SetDataModeType(svtkXMLWriterF_Table[*self], *objType);
  }
  else
  {
    fprintf(stderr, "svtkXMLWriterF_SetDataModeType called with invalid id %d.\n", *self);
  }
}

/*--------------------------------------------------------------------------*/
/* svtkXMLWriterF_SetDataObjectType */
void SVTK_FORTRAN_NAME(svtkxmlwriterf_setdataobjecttype, SVTKXMLWRITERF_SETDATAOBJECTTYPE)(
  const int* self, const int* objType)
{
  if (*self > 0 && *self <= SVTK_XMLWRITERF_MAX && svtkXMLWriterF_Table[*self])
  {
    svtkXMLWriterC_SetDataObjectType(svtkXMLWriterF_Table[*self], *objType);
  }
  else
  {
    fprintf(stderr, "svtkXMLWriterF_SetDataObjectType called with invalid id %d.\n", *self);
  }
}

/*--------------------------------------------------------------------------*/
/* svtkXMLWriterF_SetExtent */
void SVTK_FORTRAN_NAME(svtkxmlwriterf_setextent, SVTKXMLWRITERF_SETEXTENT)(
  const int* self, int extent[6])
{
  if (*self > 0 && *self <= SVTK_XMLWRITERF_MAX && svtkXMLWriterF_Table[*self])
  {
    svtkXMLWriterC_SetExtent(svtkXMLWriterF_Table[*self], extent);
  }
  else
  {
    fprintf(stderr, "svtkXMLWriterF_SetExtent called with invalid id %d.\n", *self);
  }
}

/*--------------------------------------------------------------------------*/
/* svtkXMLWriterF_SetPoints */
void SVTK_FORTRAN_NAME(svtkxmlwriterf_setpoints, SVTKXMLWRITERF_SETPOINTS)(
  const int* self, const int* dataType, void* data, const svtkIdType* numPoints)
{
  if (*self > 0 && *self <= SVTK_XMLWRITERF_MAX && svtkXMLWriterF_Table[*self])
  {
    svtkXMLWriterC_SetPoints(svtkXMLWriterF_Table[*self], *dataType, data, *numPoints);
  }
  else
  {
    fprintf(stderr, "svtkXMLWriterF_SetPoints called with invalid id %d.\n", *self);
  }
}

/*--------------------------------------------------------------------------*/
/* svtkXMLWriterF_SetOrigin */
void SVTK_FORTRAN_NAME(svtkxmlwriterf_setorigin, SVTKXMLWRITERF_SETORIGIN)(
  const int* self, double origin[3])
{
  if (*self > 0 && *self <= SVTK_XMLWRITERF_MAX && svtkXMLWriterF_Table[*self])
  {
    svtkXMLWriterC_SetOrigin(svtkXMLWriterF_Table[*self], origin);
  }
  else
  {
    fprintf(stderr, "svtkXMLWriterF_SetOrigin called with invalid id %d.\n", *self);
  }
}

/*--------------------------------------------------------------------------*/
/* svtkXMLWriterF_SetSpacing */
void SVTK_FORTRAN_NAME(svtkxmlwriterf_setspacing, SVTKXMLWRITERF_SETSPACING)(
  const int* self, double spacing[3])
{
  if (*self > 0 && *self <= SVTK_XMLWRITERF_MAX && svtkXMLWriterF_Table[*self])
  {
    svtkXMLWriterC_SetSpacing(svtkXMLWriterF_Table[*self], spacing);
  }
  else
  {
    fprintf(stderr, "svtkXMLWriterF_SetSpacing called with invalid id %d.\n", *self);
  }
}

/*--------------------------------------------------------------------------*/
/* svtkXMLWriterF_SetCoordinates */
void SVTK_FORTRAN_NAME(svtkxmlwriterf_setcoordinates, SVTKXMLWRITERF_SETCOORDINATES)(const int* self,
  const int* axis, const int* dataType, void* data, const svtkIdType* numCoordinates)
{
  if (*self > 0 && *self <= SVTK_XMLWRITERF_MAX && svtkXMLWriterF_Table[*self])
  {
    svtkXMLWriterC_SetCoordinates(
      svtkXMLWriterF_Table[*self], *axis, *dataType, data, *numCoordinates);
  }
  else
  {
    fprintf(stderr, "svtkXMLWriterF_SetCoordinates called with invalid id %d.\n", *self);
  }
}

/*--------------------------------------------------------------------------*/
/* svtkXMLWriterF_SetCellsWithType */
void SVTK_FORTRAN_NAME(svtkxmlwriterf_setcellswithtype, SVTKXMLWRITERF_SETCELLSWITHTYPE)(
  const int* self, const int* cellType, const svtkIdType* ncells, svtkIdType* cells,
  const svtkIdType* cellsSize)
{
  if (*self > 0 && *self <= SVTK_XMLWRITERF_MAX && svtkXMLWriterF_Table[*self])
  {
    svtkXMLWriterC_SetCellsWithType(
      svtkXMLWriterF_Table[*self], *cellType, *ncells, cells, *cellsSize);
  }
  else
  {
    fprintf(stderr, "svtkXMLWriterF_SetCellsWithType called with invalid id %d.\n", *self);
  }
}

/*--------------------------------------------------------------------------*/
/* svtkXMLWriterF_SetCellsWithTypes */
void SVTK_FORTRAN_NAME(svtkxmlwriterf_setcellswithtypes, SVTKXMLWRITERF_SETCELLSWITHTYPES)(
  const int* self, int* cellTypes, const svtkIdType* ncells, svtkIdType* cells,
  const svtkIdType* cellsSize)
{
  if (*self > 0 && *self <= SVTK_XMLWRITERF_MAX && svtkXMLWriterF_Table[*self])
  {
    svtkXMLWriterC_SetCellsWithTypes(
      svtkXMLWriterF_Table[*self], cellTypes, *ncells, cells, *cellsSize);
  }
  else
  {
    fprintf(stderr, "svtkXMLWriterF_SetCellsWithTypes called with invalid id %d.\n", *self);
  }
}

/*--------------------------------------------------------------------------*/
/* svtkXMLWriterF_SetPointData */
void SVTK_FORTRAN_NAME(svtkxmlwriterf_setpointdata, SVTKXMLWRITERF_SETPOINTDATA)(const int* self,
  SVTK_FORTRAN_ARG_STRING_POINTER(name), const int* dataType, void* data, const svtkIdType* numTuples,
  const int* numComponents,
  SVTK_FORTRAN_ARG_STRING_POINTER(role) SVTK_FORTRAN_ARG_STRING_LENGTH(name)
    SVTK_FORTRAN_ARG_STRING_LENGTH(role))
{
  if (*self > 0 && *self <= SVTK_XMLWRITERF_MAX && svtkXMLWriterF_Table[*self])
  {
    /* Prepare nullptr-terminated strings.  */
    const char* name_ptr = SVTK_FORTRAN_REF_STRING_POINTER(name);
    int name_length = SVTK_FORTRAN_REF_STRING_LENGTH(name);
    char* name_buffer = malloc(name_length + 1);
    const char* role_ptr = SVTK_FORTRAN_REF_STRING_POINTER(role);
    int role_length = SVTK_FORTRAN_REF_STRING_LENGTH(role);
    char* role_buffer = malloc(role_length + 1);
    if (!name_buffer || !role_buffer)
    {
      fprintf(stderr, "svtkXMLWriterF_SetPointData failed to allocate name or role.\n");
      if (name_buffer)
      {
        free(name_buffer);
      }
      if (role_buffer)
      {
        free(role_buffer);
      }
      return;
    }
    memcpy(name_buffer, name_ptr, name_length);
    name_buffer[name_length] = 0;
    memcpy(role_buffer, role_ptr, role_length);
    role_buffer[role_length] = 0;

    /* Forward the call.  */
    svtkXMLWriterC_SetPointData(svtkXMLWriterF_Table[*self], name_buffer, *dataType, data, *numTuples,
      *numComponents, role_buffer);

    /* Free the nullptr-terminated strings.  */
    free(name_buffer);
    free(role_buffer);
  }
  else
  {
    fprintf(stderr, "svtkXMLWriterF_SetPointData called with invalid id %d.\n", *self);
  }
}

/*--------------------------------------------------------------------------*/
/* svtkXMLWriterF_SetCellData */
void SVTK_FORTRAN_NAME(svtkxmlwriterf_setcelldata, SVTKXMLWRITERF_SETCELLDATA)(const int* self,
  SVTK_FORTRAN_ARG_STRING_POINTER(name), const int* dataType, void* data, const svtkIdType* numTuples,
  const int* numComponents,
  SVTK_FORTRAN_ARG_STRING_POINTER(role) SVTK_FORTRAN_ARG_STRING_LENGTH(name)
    SVTK_FORTRAN_ARG_STRING_LENGTH(role))
{
  if (*self > 0 && *self <= SVTK_XMLWRITERF_MAX && svtkXMLWriterF_Table[*self])
  {
    /* Prepare nullptr-terminated strings.  */
    const char* name_ptr = SVTK_FORTRAN_REF_STRING_POINTER(name);
    int name_length = SVTK_FORTRAN_REF_STRING_LENGTH(name);
    char* name_buffer = malloc(name_length + 1);
    const char* role_ptr = SVTK_FORTRAN_REF_STRING_POINTER(role);
    int role_length = SVTK_FORTRAN_REF_STRING_LENGTH(role);
    char* role_buffer = malloc(role_length + 1);
    if (!name_buffer || !role_buffer)
    {
      fprintf(stderr, "svtkXMLWriterF_SetCellData failed to allocate name or role.\n");
      if (name_buffer)
      {
        free(name_buffer);
      }
      if (role_buffer)
      {
        free(role_buffer);
      }
      return;
    }
    memcpy(name_buffer, name_ptr, name_length);
    name_buffer[name_length] = 0;
    memcpy(role_buffer, role_ptr, role_length);
    role_buffer[role_length] = 0;

    /* Forward the call.  */
    svtkXMLWriterC_SetCellData(svtkXMLWriterF_Table[*self], name_buffer, *dataType, data, *numTuples,
      *numComponents, role_buffer);

    /* Free the nullptr-terminated strings.  */
    free(name_buffer);
    free(role_buffer);
  }
  else
  {
    fprintf(stderr, "svtkXMLWriterF_SetCellData called with invalid id %d.\n", *self);
  }
}

/*--------------------------------------------------------------------------*/
/* svtkXMLWriterF_SetFileName */
void SVTK_FORTRAN_NAME(svtkxmlwriterf_setfilename, SVTKXMLWRITERF_SETFILENAME)(
  const int* self, SVTK_FORTRAN_ARG_STRING_POINTER(name) SVTK_FORTRAN_ARG_STRING_LENGTH(name))
{
  if (*self > 0 && *self <= SVTK_XMLWRITERF_MAX && svtkXMLWriterF_Table[*self])
  {
    /* Prepare nullptr-terminated string.  */
    const char* name_ptr = SVTK_FORTRAN_REF_STRING_POINTER(name);
    int name_length = SVTK_FORTRAN_REF_STRING_LENGTH(name);
    char* name_buffer = malloc(name_length + 1);
    if (!name_buffer)
    {
      fprintf(stderr, "svtkXMLWriterF_SetFileName failed to allocate name.\n");
      return;
    }
    memcpy(name_buffer, name_ptr, name_length);
    name_buffer[name_length] = 0;

    /* Forward the call.  */
    svtkXMLWriterC_SetFileName(svtkXMLWriterF_Table[*self], name_buffer);

    /* Free the nullptr-terminated string.  */
    free(name_buffer);
  }
  else
  {
    fprintf(stderr, "svtkXMLWriterF_SetFileName called with invalid id %d.\n", *self);
  }
}

/*--------------------------------------------------------------------------*/
/* svtkXMLWriterF_Write */
void SVTK_FORTRAN_NAME(svtkxmlwriterf_write, SVTKXMLWRITERF_WRITE)(const int* self, int* success)
{
  if (*self > 0 && *self <= SVTK_XMLWRITERF_MAX && svtkXMLWriterF_Table[*self])
  {
    *success = svtkXMLWriterC_Write(svtkXMLWriterF_Table[*self]);
  }
  else
  {
    fprintf(stderr, "svtkXMLWriterF_Write called with invalid id %d.\n", *self);
  }
}

/*--------------------------------------------------------------------------*/
/* svtkXMLWriterF_SetNumberOfTimeSteps */
void SVTK_FORTRAN_NAME(svtkxmlwriterf_setnumberoftimesteps, SVTKXMLWRITERF_SETNUMBEROFTIMESTEPS)(
  const int* self, const int* numTimeSteps)
{
  if (*self > 0 && *self <= SVTK_XMLWRITERF_MAX && svtkXMLWriterF_Table[*self])
  {
    svtkXMLWriterC_SetNumberOfTimeSteps(svtkXMLWriterF_Table[*self], *numTimeSteps);
  }
  else
  {
    fprintf(stderr, "svtkXMLWriterF_SetNumberOfTimeSteps called with invalid id %d.\n", *self);
  }
}

/*--------------------------------------------------------------------------*/
/* svtkXMLWriterF_Start */
void SVTK_FORTRAN_NAME(svtkxmlwriterf_start, SVTKXMLWRITERF_START)(const int* self)
{
  if (*self > 0 && *self <= SVTK_XMLWRITERF_MAX && svtkXMLWriterF_Table[*self])
  {
    svtkXMLWriterC_Start(svtkXMLWriterF_Table[*self]);
  }
  else
  {
    fprintf(stderr, "svtkXMLWriterF_Start called with invalid id %d.\n", *self);
  }
}

/*--------------------------------------------------------------------------*/
/* svtkXMLWriterF_WriteNextTimeStep */
void SVTK_FORTRAN_NAME(svtkxmlwriterf_writenexttimestep, SVTKXMLWRITERF_WRITENEXTTIMESTEP)(
  const int* self, const double* timeValue)
{
  if (*self > 0 && *self <= SVTK_XMLWRITERF_MAX && svtkXMLWriterF_Table[*self])
  {
    svtkXMLWriterC_WriteNextTimeStep(svtkXMLWriterF_Table[*self], *timeValue);
  }
  else
  {
    fprintf(stderr, "svtkXMLWriterF_WriteNextTimeStep called with invalid id %d.\n", *self);
  }
}

/*--------------------------------------------------------------------------*/
/* svtkXMLWriterF_Stop */
void SVTK_FORTRAN_NAME(svtkxmlwriterf_stop, SVTKXMLWRITERF_STOP)(const int* self)
{
  if (*self > 0 && *self <= SVTK_XMLWRITERF_MAX && svtkXMLWriterF_Table[*self])
  {
    svtkXMLWriterC_Stop(svtkXMLWriterF_Table[*self]);
  }
  else
  {
    fprintf(stderr, "svtkXMLWriterF_Stop called with invalid id %d.\n", *self);
  }
}
#endif
// SVTK-HeaderTest-Exclude: svtkXMLWriterF.h
