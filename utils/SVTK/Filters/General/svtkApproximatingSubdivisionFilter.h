/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkApproximatingSubdivisionFilter.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   svtkApproximatingSubdivisionFilter
 * @brief   generate a subdivision surface using an Approximating Scheme
 *
 * svtkApproximatingSubdivisionFilter is an abstract class that defines
 * the protocol for Approximating subdivision surface filters.
 *
 * @par Thanks:
 * This work was supported by PHS Research Grant No. 1 P41 RR13218-01
 * from the National Center for Research Resources.
 */

#ifndef svtkApproximatingSubdivisionFilter_h
#define svtkApproximatingSubdivisionFilter_h

#include "svtkFiltersGeneralModule.h" // For export macro
#include "svtkSubdivisionFilter.h"

class svtkCellArray;
class svtkCellData;
class svtkIdList;
class svtkIntArray;
class svtkPoints;
class svtkPointData;

class SVTKFILTERSGENERAL_EXPORT svtkApproximatingSubdivisionFilter : public svtkSubdivisionFilter
{
public:
  svtkTypeMacro(svtkApproximatingSubdivisionFilter, svtkSubdivisionFilter);
  void PrintSelf(ostream& os, svtkIndent indent) override;

protected:
  svtkApproximatingSubdivisionFilter();
  ~svtkApproximatingSubdivisionFilter() override {}

  int RequestData(svtkInformation*, svtkInformationVector**, svtkInformationVector*) override;
  virtual int GenerateSubdivisionPoints(
    svtkPolyData* inputDS, svtkIntArray* edgeData, svtkPoints* outputPts, svtkPointData* outputPD) = 0;
  void GenerateSubdivisionCells(
    svtkPolyData* inputDS, svtkIntArray* edgeData, svtkCellArray* outputPolys, svtkCellData* outputCD);
  int FindEdge(svtkPolyData* mesh, svtkIdType cellId, svtkIdType p1, svtkIdType p2,
    svtkIntArray* edgeData, svtkIdList* cellIds);
  svtkIdType InterpolatePosition(
    svtkPoints* inputPts, svtkPoints* outputPts, svtkIdList* stencil, double* weights);

private:
  svtkApproximatingSubdivisionFilter(const svtkApproximatingSubdivisionFilter&) = delete;
  void operator=(const svtkApproximatingSubdivisionFilter&) = delete;
};

#endif
