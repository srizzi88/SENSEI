/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkGeometryFilter.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   svtkGeometryFilter
 * @brief   extract geometry from data (or convert data to polygonal type)
 *
 * svtkGeometryFilter is a general-purpose filter to extract geometry (and
 * associated data) from any type of dataset. Geometry is obtained as
 * follows: all 0D, 1D, and 2D cells are extracted. All 2D faces that are
 * used by only one 3D cell (i.e., boundary faces) are extracted. It also is
 * possible to specify conditions on point ids, cell ids, and on
 * bounding box (referred to as "Extent") to control the extraction process.
 *
 * This filter also may be used to convert any type of data to polygonal
 * type. The conversion process may be less than satisfactory for some 3D
 * datasets. For example, this filter will extract the outer surface of a
 * volume or structured grid dataset. (For structured data you may want to
 * use svtkImageDataGeometryFilter, svtkStructuredGridGeometryFilter,
 * svtkExtractUnstructuredGrid, svtkRectilinearGridGeometryFilter, or
 * svtkExtractVOI.)
 *
 * @warning
 * When svtkGeometryFilter extracts cells (or boundaries of cells) it
 * will (by default) merge duplicate vertices. This may cause problems
 * in some cases. For example, if you've run svtkPolyDataNormals to
 * generate normals, which may split meshes and create duplicate
 * vertices, svtkGeometryFilter will merge these points back
 * together. Turn merging off to prevent this from occurring.
 *
 * @warning
 * This filter assumes that the input dataset is composed of either:
 * 0D cells OR 1D cells OR 2D and/or 3D cells. In other words,
 * the input dataset cannot be a combination of different dimensional cells
 * with the exception of 2D and 3D cells.
 *
 * @sa
 * svtkImageDataGeometryFilter svtkStructuredGridGeometryFilter
 * svtkExtractGeometry svtkExtractVOI
 */

#ifndef svtkGeometryFilter_h
#define svtkGeometryFilter_h

#include "svtkFiltersGeometryModule.h" // For export macro
#include "svtkPolyDataAlgorithm.h"

class svtkIncrementalPointLocator;

class SVTKFILTERSGEOMETRY_EXPORT svtkGeometryFilter : public svtkPolyDataAlgorithm
{
public:
  static svtkGeometryFilter* New();
  svtkTypeMacro(svtkGeometryFilter, svtkPolyDataAlgorithm);
  void PrintSelf(ostream& os, svtkIndent indent) override;

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

  //@{
  /**
   * Set/get the desired precision for the output types. See the documentation
   * for the svtkAlgorithm::DesiredOutputPrecision enum for an explanation of
   * the available precision settings. This only applies for data types where
   * we create points as opposed to pass them, such as rectilinear grid.
   */
  void SetOutputPointsPrecision(int precision);
  int GetOutputPointsPrecision() const;
  //@}

protected:
  svtkGeometryFilter();
  ~svtkGeometryFilter() override;

  int RequestData(svtkInformation*, svtkInformationVector**, svtkInformationVector*) override;
  int FillInputPortInformation(int port, svtkInformation* info) override;

  // special cases for performance
  void PolyDataExecute(svtkDataSet*, svtkPolyData*);
  void UnstructuredGridExecute(svtkDataSet*, svtkPolyData*);
  void StructuredGridExecute(svtkDataSet*, svtkPolyData*, svtkInformation*);
  int RequestUpdateExtent(svtkInformation*, svtkInformationVector**, svtkInformationVector*) override;

  svtkIdType PointMaximum;
  svtkIdType PointMinimum;
  svtkIdType CellMinimum;
  svtkIdType CellMaximum;
  double Extent[6];
  svtkTypeBool PointClipping;
  svtkTypeBool CellClipping;
  svtkTypeBool ExtentClipping;
  int OutputPointsPrecision;

  svtkTypeBool Merging;
  svtkIncrementalPointLocator* Locator;

private:
  svtkGeometryFilter(const svtkGeometryFilter&) = delete;
  void operator=(const svtkGeometryFilter&) = delete;
};

#endif
