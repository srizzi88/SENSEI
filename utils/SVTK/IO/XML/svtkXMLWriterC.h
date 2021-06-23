/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkXMLWriterC.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#ifndef svtkXMLWriterC_h
#define svtkXMLWriterC_h

#include "svtkIOXMLModule.h" // For export macro
#include "svtkType.h"        /* For scalar and svtkDataObject type enumerations.  */

#ifdef __cplusplus
extern "C"
{
#endif /*cplusplus*/

  /**
   * svtkXMLWriterC is an opaque structure holding the state of an
   * individual writer object.  It can be used to write SVTK XML files.
   */
  typedef struct svtkXMLWriterC_s svtkXMLWriterC;

  /**
   * Create a new instance of svtkXMLWriterC.  Returns the object or nullptr
   * on failure.
   */
  SVTKIOXML_EXPORT
  svtkXMLWriterC* svtkXMLWriterC_New(void);

  /**
   * Delete the writer object.
   *
   * This should not be called between Start and Stop calls.
   */
  SVTKIOXML_EXPORT
  void svtkXMLWriterC_Delete(svtkXMLWriterC* self);

  /**
   * Set the SVTK data object type that will be written.  This
   * initializes an empty data object of the given type.
   *
   * This must be set before setting geometry or data information can
   * can be set only once per writer object.
   */
  SVTKIOXML_EXPORT
  void svtkXMLWriterC_SetDataObjectType(svtkXMLWriterC* self, int objType);

  /**
   * Set the SVTK writer data mode to either:
   * - Ascii
   * - Binary
   * - Appended (default)
   *
   * This may be used only after SetDataObjectType has been called.
   */
  SVTKIOXML_EXPORT
  void svtkXMLWriterC_SetDataModeType(svtkXMLWriterC* self, int datamodetype);

  /**
   * Set the extent of a structured data set.
   *
   * This may be used only after SetDataObjectType has been called with
   * a structured data object type.
   */
  SVTKIOXML_EXPORT
  void svtkXMLWriterC_SetExtent(svtkXMLWriterC* self, int extent[6]);

  /**
   * Set the points of a point data set.  For structured data, the
   * number of points must match number of points indicated by the
   * extent.
   *
   * Use dataType to specify the scalar type used in the given array.
   * The data array must have numPoints*3 entries specifying 3-D points.
   *
   * This may not be used for data objects with implicit points.  It may
   * not be called before SetDataObjectType or between Start and Stop
   * calls.
   */
  SVTKIOXML_EXPORT
  void svtkXMLWriterC_SetPoints(svtkXMLWriterC* self, int dataType, void* data, svtkIdType numPoints);

  /**
   * Set the origin of an image data set.
   *
   * This may only be used for image data.  It may not be called before
   * SetDataObjectType or between Start and Stop calls.
   */
  SVTKIOXML_EXPORT
  void svtkXMLWriterC_SetOrigin(svtkXMLWriterC* self, double origin[3]);

  /**
   * Set the spacing of an image data set.
   *
   * This may only be used for image data.  It may not be called before
   * SetDataObjectType or between Start and Stop calls.
   */
  SVTKIOXML_EXPORT
  void svtkXMLWriterC_SetSpacing(svtkXMLWriterC* self, double spacing[3]);

  /**
   * Set the coordinates along one axis of a rectilinear grid data set.
   *
   * Specify axis 0 for X, 1 for Y, and 2 for Z.  Use dataType to
   * specify the scalar type used in the given data array.  Use
   * numCoordinates to specify the number of such values in the array.
   * The number of values must match that specified by the extent for
   * the given axis.
   *
   * This may only be used for rectilinear grids.  It may not be called
   * before SetDataObjectType or between Start and Stop calls.
   */
  SVTKIOXML_EXPORT
  void svtkXMLWriterC_SetCoordinates(
    svtkXMLWriterC* self, int axis, int dataType, void* data, svtkIdType numCoordinates);

  /**
   * Set a cell array on the data object to be written.  All cells must
   * have the same type.
   *
   * For unstructured grid data objects, the cellType can be any type.
   * For polygonal data objects, the cellType must be SVTK_VERTEX,
   * SVTK_POLY_VERTEX, SVTK_LINE, SVTK_POLY_LINE, SVTK_TRIANGLE,
   * SVTK_TRIANGLE_STRIP, or cyclically connected simple cell type such
   * as SVTK_POLYGON.
   *
   * The cells array must have cellsSize entries.  Each cell uses N+1
   * entries where N is the number of points in the cell.  The layout of
   * the array for each cell is "[N,id1,id2,...,idN]".  The total number
   * of cells must be ncells.
   *
   * This may only be used for unstructured grid and polygonal data
   * types.  It may not be called before SetDataObjectType or between
   * Start and Stop calls.
   */
  SVTKIOXML_EXPORT
  void svtkXMLWriterC_SetCellsWithType(
    svtkXMLWriterC* self, int cellType, svtkIdType ncells, svtkIdType* cells, svtkIdType cellsSize);

  /**
   * Set a cell array on the data object to be written.  Each cell can
   * have its own type.
   *
   * The cellTypes array specifies the type of each cell, and has ncells
   * entries.  The cells array must have cellsSize entries.  Each cell
   * uses N+1 entries where N is the number of points in the cell.  The
   * layout of the array for each cell is "[N,id1,id2,...,idN]".  The
   * total number of cells must be ncells.
   *
   * This may only be used for unstructured grid data objects.  It may
   * not be called before SetDataObjectType or between Start and Stop
   * calls.
   */
  SVTKIOXML_EXPORT
  void svtkXMLWriterC_SetCellsWithTypes(
    svtkXMLWriterC* self, int* cellTypes, svtkIdType ncells, svtkIdType* cells, svtkIdType cellsSize);

  /**
   * Set a point or cell data array by name.
   *
   * The name of the array is required and should describe the purpose
   * of the data.  Use dataType to specify the scalar type used in the
   * given data array.  Use numTuples to specify the number of tuples
   * and numComponents to specify the number of scalar components in
   * each tuple.
   *
   * The data array must have exactly numTuples*numComponents entries.
   * For SetPointData, numTuples must be equal to the number of points
   * indicated by SetExtent and/or SetPoints.  For SetCellData,
   * numTuples must be equal to the total number of cells set by
   * SetCells.
   *
   * The role can be one of "SCALARS", "VECTORS", "NORMALS", "TENSORS",
   * or "TCOORDS" and specifies that the array should be designated as
   * the active array for the named role.  Other values for role are
   * ignored.
   *
   * This may be used for all data types.  It may not be called before
   * SetDataObjectType but may be called between Start and Stop calls.
   */
  SVTKIOXML_EXPORT
  void svtkXMLWriterC_SetPointData(svtkXMLWriterC* self, const char* name, int dataType, void* data,
    svtkIdType numTuples, int numComponents, const char* role);
  SVTKIOXML_EXPORT
  void svtkXMLWriterC_SetCellData(svtkXMLWriterC* self, const char* name, int dataType, void* data,
    svtkIdType numTuples, int numComponents, const char* role);

  /**
   * Set the name of the file into which the data are to be written.
   *
   * This may be used for all data types.  It may not be called before
   * SetDataObjectType or between Start and Stop calls.
   */
  SVTKIOXML_EXPORT
  void svtkXMLWriterC_SetFileName(svtkXMLWriterC* self, const char* fileName);

  /**
   * Write the data to a file immediately.  This is not used when
   * writing time-series data.  Returns 1 for success and 0 for failure.
   *
   * This may only be called after SetFileName and SetDataObjectType.
   */
  SVTKIOXML_EXPORT
  int svtkXMLWriterC_Write(svtkXMLWriterC* self);

  /**
   * Set the number of time steps that will be written between upcoming
   * Start and Stop calls.  This is used when writing time-series data.
   *
   * This may be used for all data types.  It may not be called before
   * SetDataObjectType or between Start and Stop calls.
   */
  SVTKIOXML_EXPORT
  void svtkXMLWriterC_SetNumberOfTimeSteps(svtkXMLWriterC* self, int numTimeSteps);

  /**
   * Start writing a time-series to the output file.
   *
   * This may only be called after SetFileName, SetDataObjectType, and
   * SetNumberOfTimeSteps.  It may not be called a second time until
   * after an intervening call to Stop.
   */
  SVTKIOXML_EXPORT
  void svtkXMLWriterC_Start(svtkXMLWriterC* self);

  /**
   * Write one time step of a time-series to the output file.  The
   * current data set by SetPointData and SetCellData will be written.
   *
   * Use timeValue to specify the time associated with the time step
   * being written.
   *
   * This may only be called after Start has been called.  It should be
   * called NumberOfTimeSteps times before calling Stop.
   */
  SVTKIOXML_EXPORT
  void svtkXMLWriterC_WriteNextTimeStep(svtkXMLWriterC* self, double timeValue);

  /**
   * Stop writing a time-series to the output file.
   *
   * This may only be called after Start and NumberOfTimeSteps calls to
   * WriteNextTimeStep.
   */
  SVTKIOXML_EXPORT
  void svtkXMLWriterC_Stop(svtkXMLWriterC* self);

#ifdef __cplusplus
} /* extern "C" */
#endif /*cplusplus*/

#endif
// SVTK-HeaderTest-Exclude: svtkXMLWriterC.h
