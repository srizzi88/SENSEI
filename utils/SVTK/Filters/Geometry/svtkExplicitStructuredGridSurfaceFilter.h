/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkExplicitStructuredGridSurfaceFilter.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   svtkExplicitStructuredGridSurfaceFilter
 * @brief   Filter which creates a surface (polydata) from an explicit structured grid.
 */

#ifndef svtkExplicitStructuredGridSurfaceFilter_h
#define svtkExplicitStructuredGridSurfaceFilter_h

#include "svtkFiltersGeometryModule.h" // For export macro
#include "svtkPolyDataAlgorithm.h"

class svtkExplicitStructuredGrid;
class svtkIdTypeArray;
class svtkMultiProcessController;

class SVTKFILTERSGEOMETRY_EXPORT svtkExplicitStructuredGridSurfaceFilter : public svtkPolyDataAlgorithm
{
public:
  static svtkExplicitStructuredGridSurfaceFilter* New();
  svtkTypeMacro(svtkExplicitStructuredGridSurfaceFilter, svtkPolyDataAlgorithm);
  void PrintSelf(ostream& os, svtkIndent indent) override;

  //@{
  /**
   * If on, the output polygonal dataset will have a celldata array that
   * holds the cell index of the original 3D cell that produced each output
   * cell. This is useful for cell picking. The default is off to conserve
   * memory. Note that PassThroughCellIds will be ignored if UseStrips is on,
   * since in that case each tringle strip can represent more than on of the
   * input cells.
   */
  svtkSetMacro(PassThroughCellIds, int);
  svtkGetMacro(PassThroughCellIds, int);
  svtkBooleanMacro(PassThroughCellIds, int);
  svtkSetMacro(PassThroughPointIds, int);
  svtkGetMacro(PassThroughPointIds, int);
  svtkBooleanMacro(PassThroughPointIds, int);
  //@}

  //@{
  /**
   * If PassThroughCellIds or PassThroughPointIds is on, then these ivars
   * control the name given to the field in which the ids are written into.  If
   * set to NULL, then svtkOriginalCellIds or svtkOriginalPointIds (the default)
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

protected:
  svtkExplicitStructuredGridSurfaceFilter();
  ~svtkExplicitStructuredGridSurfaceFilter() override;

  int RequestInformation(svtkInformation*, svtkInformationVector**, svtkInformationVector*) override;
  int RequestUpdateExtent(svtkInformation*, svtkInformationVector**, svtkInformationVector*) override;
  int RequestData(svtkInformation*, svtkInformationVector**, svtkInformationVector*) override;
  int FillInputPortInformation(int, svtkInformation*) override;

  int ExtractSurface(svtkExplicitStructuredGrid*, svtkPolyData*);

  // Helper methods.

  int PieceInvariant;

  int PassThroughCellIds;
  char* OriginalCellIdsName;

  int PassThroughPointIds;
  char* OriginalPointIdsName;

  int WholeExtent[6];

private:
  svtkExplicitStructuredGridSurfaceFilter(const svtkExplicitStructuredGridSurfaceFilter&) = delete;
  void operator=(const svtkExplicitStructuredGridSurfaceFilter&) = delete;
};

#endif
