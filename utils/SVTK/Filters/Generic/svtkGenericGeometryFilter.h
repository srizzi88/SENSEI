/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkGenericGeometryFilter.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   svtkGenericGeometryFilter
 * @brief   extract geometry from data (or convert data to polygonal type)
 *
 * svtkGenericGeometryFilter is a general-purpose filter to extract geometry (and
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
 * When svtkGenericGeometryFilter extracts cells (or boundaries of cells) it
 * will (by default) merge duplicate vertices. This may cause problems
 * in some cases. For example, if you've run svtkPolyDataNormals to
 * generate normals, which may split meshes and create duplicate
 * vertices, svtkGenericGeometryFilter will merge these points back
 * together. Turn merging off to prevent this from occurring.
 *
 * @sa
 * svtkImageDataGeometryFilter svtkStructuredGridGeometryFilter
 * svtkExtractGeometry svtkExtractVOI
 */

#ifndef svtkGenericGeometryFilter_h
#define svtkGenericGeometryFilter_h

#include "svtkFiltersGenericModule.h" // For export macro
#include "svtkPolyDataAlgorithm.h"

class svtkIncrementalPointLocator;
class svtkPointData;

class SVTKFILTERSGENERIC_EXPORT svtkGenericGeometryFilter : public svtkPolyDataAlgorithm
{
public:
  static svtkGenericGeometryFilter* New();
  svtkTypeMacro(svtkGenericGeometryFilter, svtkPolyDataAlgorithm);
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
  double* GetExtent() { return this->Extent; }
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
   * If on, the output polygonal dataset will have a celldata array that
   * holds the cell index of the original 3D cell that produced each output
   * cell. This is useful for cell picking. The default is off to conserve
   * memory.
   */
  svtkSetMacro(PassThroughCellIds, svtkTypeBool);
  svtkGetMacro(PassThroughCellIds, svtkTypeBool);
  svtkBooleanMacro(PassThroughCellIds, svtkTypeBool);
  //@}

protected:
  svtkGenericGeometryFilter();
  ~svtkGenericGeometryFilter() override;

  int RequestData(svtkInformation*, svtkInformationVector**, svtkInformationVector*) override;
  void PolyDataExecute(); // special cases for performance
  void UnstructuredGridExecute();
  void StructuredGridExecute();
  int RequestUpdateExtent(svtkInformation*, svtkInformationVector**, svtkInformationVector*) override;

  int FillInputPortInformation(int, svtkInformation*) override;

  svtkIdType PointMaximum;
  svtkIdType PointMinimum;
  svtkIdType CellMinimum;
  svtkIdType CellMaximum;
  double Extent[6];
  svtkTypeBool PointClipping;
  svtkTypeBool CellClipping;
  svtkTypeBool ExtentClipping;

  svtkTypeBool Merging;
  svtkIncrementalPointLocator* Locator;

  // Used internal by svtkGenericAdaptorCell::Tessellate()
  svtkPointData* InternalPD;

  svtkTypeBool PassThroughCellIds;

private:
  svtkGenericGeometryFilter(const svtkGenericGeometryFilter&) = delete;
  void operator=(const svtkGenericGeometryFilter&) = delete;
};

#endif
