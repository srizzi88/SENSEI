/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkObject.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   svtkMatrixMathFilter
 * @brief   Calculate functions of quality of the elements
 *  of a mesh
 *
 *
 * svtkMatrixMathFilter computes one or more functions of mathematical quality for the
 * cells or points in a mesh. The per-cell or per-point quality is added to the
 * mesh's cell data or point data, in an array with names varied with different
 * quality being queried. Note this filter always assume the data associate with
 * the cells or points are 3 by 3 matrix.
 */

#ifndef svtkMatrixMathFilter_h
#define svtkMatrixMathFilter_h

#include "svtkDataSetAlgorithm.h"
#include "svtkFiltersVerdictModule.h" // For export macro

class svtkCell;
class svtkDataArray;

class SVTKFILTERSVERDICT_EXPORT svtkMatrixMathFilter : public svtkDataSetAlgorithm
{

  enum
  {
    NONE = 0,
    DETERMINANT,
    EIGENVALUE,
    EIGENVECTOR,
    INVERSE
  };
  enum
  {
    POINT_QUALITY = 0,
    CELL_QUALITY
  };

public:
  void PrintSelf(ostream&, svtkIndent) override;
  svtkTypeMacro(svtkMatrixMathFilter, svtkDataSetAlgorithm);
  static svtkMatrixMathFilter* New();

  //@{
  /**
   * Set/Get the particular estimator used to function the quality of query.
   */
  svtkSetMacro(Operation, int);
  svtkGetMacro(Operation, int);
  void SetOperationToDeterminant() { this->SetOperation(DETERMINANT); }
  void SetOperationToEigenvalue() { this->SetOperation(EIGENVALUE); }
  void SetOperationToEigenvector() { this->SetOperation(EIGENVECTOR); }
  void SetOperationToInverse() { this->SetOperation(INVERSE); }
  //@}

protected:
  ~svtkMatrixMathFilter() override;
  svtkMatrixMathFilter();

  int RequestData(svtkInformation*, svtkInformationVector**, svtkInformationVector*) override;

  int Operation;

private:
  svtkMatrixMathFilter(const svtkMatrixMathFilter&) = delete;
  void operator=(const svtkMatrixMathFilter&) = delete;
};

#endif // svtkMatrixMathFilter_h
