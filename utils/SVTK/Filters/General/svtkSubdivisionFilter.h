/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkSubdivisionFilter.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   svtkSubdivisionFilter
 * @brief   base class for subvision filters
 *
 * svtkSubdivisionFilter is an abstract class that defines
 * the protocol for subdivision surface filters.
 *
 */

#ifndef svtkSubdivisionFilter_h
#define svtkSubdivisionFilter_h

#include "svtkFiltersGeneralModule.h" // For export macro
#include "svtkPolyDataAlgorithm.h"

class svtkCellArray;
class svtkCellData;
class svtkIdList;
class svtkIntArray;
class svtkPoints;
class svtkPointData;

class SVTKFILTERSGENERAL_EXPORT svtkSubdivisionFilter : public svtkPolyDataAlgorithm
{
public:
  svtkTypeMacro(svtkSubdivisionFilter, svtkPolyDataAlgorithm);
  void PrintSelf(ostream& os, svtkIndent indent) override;

  //@{
  /**
   * Set/get the number of subdivisions.
   * Default is 1.
   */
  svtkSetMacro(NumberOfSubdivisions, int);
  svtkGetMacro(NumberOfSubdivisions, int);
  //@}

  //@{
  /**
   * Set/get CheckForTriangles
   * Should subdivision check that the dataset only contains triangles?
   * Default is On (1).
   */
  svtkSetClampMacro(CheckForTriangles, svtkTypeBool, 0, 1);
  svtkGetMacro(CheckForTriangles, svtkTypeBool);
  svtkBooleanMacro(CheckForTriangles, svtkTypeBool);
  //@}

protected:
  svtkSubdivisionFilter();
  ~svtkSubdivisionFilter() override {}

  int RequestData(svtkInformation*, svtkInformationVector**, svtkInformationVector*) override;

  int NumberOfSubdivisions;
  svtkTypeBool CheckForTriangles;

private:
  svtkSubdivisionFilter(const svtkSubdivisionFilter&) = delete;
  void operator=(const svtkSubdivisionFilter&) = delete;
};

#endif
