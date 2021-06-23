/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkExtractUnstructuredGrid.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   svtkExtractUnstructuredGrid
 * @brief   extract subset of unstructured grid geometry
 *
 * svtkExtractUnstructuredGrid is a general-purpose filter to
 * extract geometry (and associated data) from an unstructured grid
 * dataset. The extraction process is controlled by specifying a range
 * of point ids, cell ids, or a bounding box (referred to as "Extent").
 * Those cells laying within these regions are sent to the output.
 * The user has the choice of merging coincident points (Merging is on)
 * or using the original point set (Merging is off).
 *
 * @warning
 * If merging is off, the input points are copied through to the
 * output. This means unused points may be present in the output data.
 * If merging is on, then coincident points with different point attribute
 * values are merged.
 *
 * @sa
 * svtkImageDataGeometryFilter svtkStructuredGridGeometryFilter
 * svtkRectilinearGridGeometryFilter
 * svtkExtractGeometry svtkExtractVOI
 */

#ifndef svtkExtractUnstructuredGrid_h
#define svtkExtractUnstructuredGrid_h

#include "svtkFiltersExtractionModule.h" // For export macro
#include "svtkUnstructuredGridAlgorithm.h"

class svtkIncrementalPointLocator;

class SVTKFILTERSEXTRACTION_EXPORT svtkExtractUnstructuredGrid : public svtkUnstructuredGridAlgorithm
{
public:
  svtkTypeMacro(svtkExtractUnstructuredGrid, svtkUnstructuredGridAlgorithm);
  void PrintSelf(ostream& os, svtkIndent indent) override;

  /**
   * Construct with all types of clipping turned off.
   */
  static svtkExtractUnstructuredGrid* New();

  //@{
  /**
   * Turn on/off selection of geometry by point id.
   */
  svtkSetMacro(PointClipping, svtkTypeBool);
  svtkGetMacro(PointClipping, svtkTypeBool);
  svtkBooleanMacro(PointClipping, svtkTypeBool);
  //@}

  //@{
  /**
   * Turn on/off selection of geometry by cell id.
   */
  svtkSetMacro(CellClipping, svtkTypeBool);
  svtkGetMacro(CellClipping, svtkTypeBool);
  svtkBooleanMacro(CellClipping, svtkTypeBool);
  //@}

  //@{
  /**
   * Turn on/off selection of geometry via bounding box.
   */
  svtkSetMacro(ExtentClipping, svtkTypeBool);
  svtkGetMacro(ExtentClipping, svtkTypeBool);
  svtkBooleanMacro(ExtentClipping, svtkTypeBool);
  //@}

  //@{
  /**
   * Specify the minimum point id for point id selection.
   */
  svtkSetClampMacro(PointMinimum, svtkIdType, 0, SVTK_ID_MAX);
  svtkGetMacro(PointMinimum, svtkIdType);
  //@}

  //@{
  /**
   * Specify the maximum point id for point id selection.
   */
  svtkSetClampMacro(PointMaximum, svtkIdType, 0, SVTK_ID_MAX);
  svtkGetMacro(PointMaximum, svtkIdType);
  //@}

  //@{
  /**
   * Specify the minimum cell id for point id selection.
   */
  svtkSetClampMacro(CellMinimum, svtkIdType, 0, SVTK_ID_MAX);
  svtkGetMacro(CellMinimum, svtkIdType);
  //@}

  //@{
  /**
   * Specify the maximum cell id for point id selection.
   */
  svtkSetClampMacro(CellMaximum, svtkIdType, 0, SVTK_ID_MAX);
  svtkGetMacro(CellMaximum, svtkIdType);
  //@}

  /**
   * Specify a (xmin,xmax, ymin,ymax, zmin,zmax) bounding box to clip data.
   */
  void SetExtent(double xMin, double xMax, double yMin, double yMax, double zMin, double zMax);

  //@{
  /**
   * Set / get a (xmin,xmax, ymin,ymax, zmin,zmax) bounding box to clip data.
   */
  void SetExtent(double extent[6]);
  double* GetExtent() SVTK_SIZEHINT(6) { return this->Extent; }
  //@}

  //@{
  /**
   * Turn on/off merging of coincident points. Note that is merging is
   * on, points with different point attributes (e.g., normals) are merged,
   * which may cause rendering artifacts.
   */
  svtkSetMacro(Merging, svtkTypeBool);
  svtkGetMacro(Merging, svtkTypeBool);
  svtkBooleanMacro(Merging, svtkTypeBool);
  //@}

  //@{
  /**
   * Set / get a spatial locator for merging points. By
   * default an instance of svtkMergePoints is used.
   */
  void SetLocator(svtkIncrementalPointLocator* locator);
  svtkGetObjectMacro(Locator, svtkIncrementalPointLocator);
  //@}

  /**
   * Create default locator. Used to create one when none is specified.
   */
  void CreateDefaultLocator();

  /**
   * Return the MTime also considering the locator.
   */
  svtkMTimeType GetMTime() override;

protected:
  svtkExtractUnstructuredGrid();
  ~svtkExtractUnstructuredGrid() override {}

  int RequestData(svtkInformation*, svtkInformationVector**, svtkInformationVector*) override;

  svtkIdType PointMinimum;
  svtkIdType PointMaximum;
  svtkIdType CellMinimum;
  svtkIdType CellMaximum;
  double Extent[6];
  svtkTypeBool PointClipping;
  svtkTypeBool CellClipping;
  svtkTypeBool ExtentClipping;

  svtkTypeBool Merging;
  svtkIncrementalPointLocator* Locator;

private:
  svtkExtractUnstructuredGrid(const svtkExtractUnstructuredGrid&) = delete;
  void operator=(const svtkExtractUnstructuredGrid&) = delete;
};

#endif
