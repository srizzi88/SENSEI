/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkUnstructuredGridGeometryFilter.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   svtkUnstructuredGridGeometryFilter
 * @brief   extract geometry from an unstructured grid
 *
 * svtkUnstructuredGridGeometryFilter is a filter that extracts
 * geometry (and associated data) from an unstructured grid. It differs from
 * svtkGeometryFilter by not tessellating higher order faces: 2D faces of
 * quadratic 3D cells will be quadratic. A quadratic edge is extracted as a
 * quadratic edge. For that purpose, the output of this filter is an
 * unstructured grid, not a polydata.
 * Also, the face of a voxel is a pixel, not a quad.
 * Geometry is obtained as follows: all 0D, 1D, and 2D cells are extracted.
 * All 2D faces that are used by only one 3D cell (i.e., boundary faces) are
 * extracted. It also is possible to specify conditions on point ids, cell ids,
 * and on bounding box (referred to as "Extent") to control the extraction
 * process.
 *
 * @warning
 * When svtkUnstructuredGridGeometryFilter extracts cells (or boundaries of
 * cells) it will (by default) merge duplicate vertices. This may cause
 * problems in some cases. Turn merging off to prevent this from occurring.
 *
 * @sa
 * svtkGeometryFilter
 */

#ifndef svtkUnstructuredGridGeometryFilter_h
#define svtkUnstructuredGridGeometryFilter_h

#include "svtkFiltersGeometryModule.h" // For export macro
#include "svtkUnstructuredGridBaseAlgorithm.h"

class svtkIncrementalPointLocator;
class svtkHashTableOfSurfels; // internal class

class SVTKFILTERSGEOMETRY_EXPORT svtkUnstructuredGridGeometryFilter
  : public svtkUnstructuredGridBaseAlgorithm
{
public:
  static svtkUnstructuredGridGeometryFilter* New();
  svtkTypeMacro(svtkUnstructuredGridGeometryFilter, svtkUnstructuredGridBaseAlgorithm);
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
   * Turn on/off clipping of ghost cells with type
   * svtkDataSetAttributes::DUPLICATECELL. Defaults to on.
   */
  svtkSetMacro(DuplicateGhostCellClipping, svtkTypeBool);
  svtkGetMacro(DuplicateGhostCellClipping, svtkTypeBool);
  svtkBooleanMacro(DuplicateGhostCellClipping, svtkTypeBool);
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
   * If on, the output polygonal dataset will have a celldata array that
   * holds the cell index of the original 3D cell that produced each output
   * cell. This is useful for cell picking. The default is off to conserve
   * memory. Note that PassThroughCellIds will be ignored if UseStrips is on,
   * since in that case each tringle strip can represent more than on of the
   * input cells.
   */
  svtkSetMacro(PassThroughCellIds, svtkTypeBool);
  svtkGetMacro(PassThroughCellIds, svtkTypeBool);
  svtkBooleanMacro(PassThroughCellIds, svtkTypeBool);
  svtkSetMacro(PassThroughPointIds, svtkTypeBool);
  svtkGetMacro(PassThroughPointIds, svtkTypeBool);
  svtkBooleanMacro(PassThroughPointIds, svtkTypeBool);
  //@}

  //@{
  /**
   * If PassThroughCellIds or PassThroughPointIds is on, then these ivars
   * control the name given to the field in which the ids are written into.  If
   * set to nullptr, then svtkOriginalCellIds or svtkOriginalPointIds (the default)
   * is used, respectively.
   */
  svtkSetStringMacro(OriginalCellIdsName);
  virtual const char* GetOriginalCellIdsName()
  {
    return (this->OriginalCellIdsName ? this->OriginalCellIdsName : "svtkOriginalCellIds");
  }
  svtkSetStringMacro(OriginalPointIdsName);
  virtual const char* GetOriginalPointIdsName()
  {
    return (this->OriginalPointIdsName ? this->OriginalPointIdsName : "svtkOriginalPointIds");
  }
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
  svtkUnstructuredGridGeometryFilter();
  ~svtkUnstructuredGridGeometryFilter() override;

  int RequestData(svtkInformation*, svtkInformationVector**, svtkInformationVector*) override;
  int FillInputPortInformation(int port, svtkInformation* info) override;

  int RequestUpdateExtent(svtkInformation*, svtkInformationVector**, svtkInformationVector*) override;

  svtkIdType PointMaximum;
  svtkIdType PointMinimum;
  svtkIdType CellMinimum;
  svtkIdType CellMaximum;
  double Extent[6];
  svtkTypeBool PointClipping;
  svtkTypeBool CellClipping;
  svtkTypeBool ExtentClipping;
  svtkTypeBool DuplicateGhostCellClipping;

  svtkTypeBool PassThroughCellIds;
  svtkTypeBool PassThroughPointIds;
  char* OriginalCellIdsName;
  char* OriginalPointIdsName;

  svtkTypeBool Merging;
  svtkIncrementalPointLocator* Locator;

  svtkHashTableOfSurfels* HashTable;

private:
  svtkUnstructuredGridGeometryFilter(const svtkUnstructuredGridGeometryFilter&) = delete;
  void operator=(const svtkUnstructuredGridGeometryFilter&) = delete;
};

#endif
