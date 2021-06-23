/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkIdFilter.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   svtkIdFilter
 * @brief   generate scalars or field data from point and cell ids
 *
 * svtkIdFilter is a filter to that generates scalars or field data
 * using cell and point ids. That is, the point attribute data scalars
 * or field data are generated from the point ids, and the cell
 * attribute data scalars or field data are generated from the
 * cell ids.
 *
 * Typically this filter is used with svtkLabeledDataMapper (and possibly
 * svtkSelectVisiblePoints) to create labels for points and cells, or labels
 * for the point or cell data scalar values.
 */

#ifndef svtkIdFilter_h
#define svtkIdFilter_h

#include "svtkDataSetAlgorithm.h"
#include "svtkFiltersCoreModule.h" // For export macro

class SVTKFILTERSCORE_EXPORT svtkIdFilter : public svtkDataSetAlgorithm
{
public:
  svtkTypeMacro(svtkIdFilter, svtkDataSetAlgorithm);
  void PrintSelf(ostream& os, svtkIndent indent) override;

  /**
   * Construct object with PointIds and CellIds on; and ids being generated
   * as scalars.
   */
  static svtkIdFilter* New();

  //@{
  /**
   * Enable/disable the generation of point ids. Default is on.
   */
  svtkSetMacro(PointIds, svtkTypeBool);
  svtkGetMacro(PointIds, svtkTypeBool);
  svtkBooleanMacro(PointIds, svtkTypeBool);
  //@}

  //@{
  /**
   * Enable/disable the generation of point ids. Default is on.
   */
  svtkSetMacro(CellIds, svtkTypeBool);
  svtkGetMacro(CellIds, svtkTypeBool);
  svtkBooleanMacro(CellIds, svtkTypeBool);
  //@}

  //@{
  /**
   * Set/Get the flag which controls whether to generate scalar data
   * or field data. If this flag is off, scalar data is generated.
   * Otherwise, field data is generated. Default is off.
   */
  svtkSetMacro(FieldData, svtkTypeBool);
  svtkGetMacro(FieldData, svtkTypeBool);
  svtkBooleanMacro(FieldData, svtkTypeBool);
  //@}

  //@{
  /**
   * @deprecated use SetPointIdsArrayName/GetPointIdsArrayName or
   * SetCellIdsArrayName/GetCellIdsArrayName.
   */
  SVTK_LEGACY(void SetIdsArrayName(const char*));
  SVTK_LEGACY(const char* GetIdsArrayName());
  //@}

  //@{
  /**
   * Set/Get the name of the Ids array for points, if generated. By default,
   * set to "svtkIdFilter_Ids" for backwards compatibility.
   */
  svtkSetStringMacro(PointIdsArrayName);
  svtkGetStringMacro(PointIdsArrayName);
  //@}

  //@{
  /**
   * Set/Get the name of the Ids array for points, if generated. By default,
   * set to "svtkIdFilter_Ids" for backwards compatibility.
   */
  svtkSetStringMacro(CellIdsArrayName);
  svtkGetStringMacro(CellIdsArrayName);
  //@}
protected:
  svtkIdFilter();
  ~svtkIdFilter() override;

  int RequestData(svtkInformation*, svtkInformationVector**, svtkInformationVector*) override;

  svtkTypeBool PointIds;
  svtkTypeBool CellIds;
  svtkTypeBool FieldData;
  char* PointIdsArrayName;
  char* CellIdsArrayName;

private:
  svtkIdFilter(const svtkIdFilter&) = delete;
  void operator=(const svtkIdFilter&) = delete;
};

#endif
