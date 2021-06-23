//=========================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//=========================================================================
/**
 * @class   svtkDataSetRegionSurfaceFilter
 * @brief   Extract surface of materials.
 *
 * This filter extracts surfaces of materials such that a surface
 * could have a material on each side of it. It also stores a
 * mapping of the original cells and their sides back to the original grid
 * so that we can output boundary information for those cells given
 * only surfaces.
 */

#ifndef svtkDataSetRegionSurfaceFilter_h
#define svtkDataSetRegionSurfaceFilter_h

#include "svtkFiltersGeometryModule.h" // For export macro

#include "svtkDataSetSurfaceFilter.h"

class svtkCharArray;

class SVTKFILTERSGEOMETRY_EXPORT svtkDataSetRegionSurfaceFilter : public svtkDataSetSurfaceFilter
{
public:
  static svtkDataSetRegionSurfaceFilter* New();
  svtkTypeMacro(svtkDataSetRegionSurfaceFilter, svtkDataSetSurfaceFilter);
  void PrintSelf(ostream& os, svtkIndent indent) override;

  //@{
  /**
   * The name of the cell based array that we use to extract interfaces from
   * Default is "Regions"
   */
  svtkSetStringMacro(RegionArrayName);
  svtkGetStringMacro(RegionArrayName);
  //@}

  int UnstructuredGridExecute(svtkDataSet* input, svtkPolyData* output) override;

  // make it clear we want all the recordOrigCellId signatures from our parent
  using svtkDataSetSurfaceFilter::RecordOrigCellId;

  // override one of the signatures
  void RecordOrigCellId(svtkIdType newIndex, svtkFastGeomQuad* quad) override;

  //@{
  /**
   * Whether to return single sided material interfaces or double sided
   * Default is single
   */
  svtkSetMacro(SingleSided, bool);
  svtkGetMacro(SingleSided, bool);
  //@}

  //@{
  /**
   * The name of the field array that has characteristics of each material
   * Default is "material_properties"
   */
  svtkSetStringMacro(MaterialPropertiesName);
  svtkGetStringMacro(MaterialPropertiesName);
  //@}

  //@{
  /**
   * The name of the field array that has material type identifiers in it
   * Default is "material_ids"
   */
  svtkSetStringMacro(MaterialIDsName);
  svtkGetStringMacro(MaterialIDsName);
  //@}

  //@{
  /**
   * The name of the output field array that records parent materials of each interface
   * Default is "material_ancestors"
   */
  svtkSetStringMacro(MaterialPIDsName);
  svtkGetStringMacro(MaterialPIDsName);
  //@}

  //@{
  /**
   * The name of the field array that has material interface type identifiers in it
   * Default is "interface_ids"
   */
  svtkSetStringMacro(InterfaceIDsName);
  svtkGetStringMacro(InterfaceIDsName);
  //@}

protected:
  svtkDataSetRegionSurfaceFilter();
  ~svtkDataSetRegionSurfaceFilter() override;

  int FillInputPortInformation(int port, svtkInformation* info) override;

  /// Implementation of the algorithm.
  int RequestData(svtkInformation*, svtkInformationVector**, svtkInformationVector*) override;

  virtual void InsertQuadInHash(
    svtkIdType a, svtkIdType b, svtkIdType c, svtkIdType d, svtkIdType sourceId, svtkIdType faceId);
  void InsertQuadInHash(
    svtkIdType a, svtkIdType b, svtkIdType c, svtkIdType d, svtkIdType sourceId) override
  {
    this->InsertQuadInHash(a, b, c, d, sourceId, -1); // for -Woverloaded-virtual comp warning
  }

  void InsertTriInHash(
    svtkIdType a, svtkIdType b, svtkIdType c, svtkIdType sourceId, svtkIdType faceId) override;
  virtual void InsertTriInHash(svtkIdType a, svtkIdType b, svtkIdType c, svtkIdType sourceId)
  {
    this->InsertTriInHash(a, b, c, sourceId, -1); // for -Woverloaded-virtual comp warning
  }

  virtual svtkFastGeomQuad* GetNextVisibleQuadFromHash();

private:
  svtkDataSetRegionSurfaceFilter(const svtkDataSetRegionSurfaceFilter&) = delete;
  void operator=(const svtkDataSetRegionSurfaceFilter&) = delete;

  char* RegionArrayName;
  svtkIntArray* RegionArray;
  svtkIdTypeArray* OrigCellIds;
  svtkCharArray* CellFaceIds;
  bool SingleSided;
  char* MaterialPropertiesName;
  char* MaterialIDsName;
  char* MaterialPIDsName;
  char* InterfaceIDsName;

  class Internals;
  Internals* Internal;
};

#endif
