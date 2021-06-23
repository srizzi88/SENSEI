/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkNormalizeMatrixVectors.h

-------------------------------------------------------------------------
  Copyright 2008 Sandia Corporation.
  Under the terms of Contract DE-AC04-94AL85000 with Sandia Corporation,
  the U.S. Government retains certain rights in this software.
-------------------------------------------------------------------------

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

/**
 * @class   svtkNormalizeMatrixVectors
 * @brief   given a sparse input matrix, produces
 * a sparse output matrix with each vector normalized to unit length with respect
 * to a p-norm (default p=2).
 *
 * @par Thanks:
 * Developed by Timothy M. Shead (tshead@sandia.gov) at Sandia National Laboratories.
 */

#ifndef svtkNormalizeMatrixVectors_h
#define svtkNormalizeMatrixVectors_h

#include "svtkArrayDataAlgorithm.h"
#include "svtkFiltersGeneralModule.h" // For export macro

class SVTKFILTERSGENERAL_EXPORT svtkNormalizeMatrixVectors : public svtkArrayDataAlgorithm
{
public:
  static svtkNormalizeMatrixVectors* New();
  svtkTypeMacro(svtkNormalizeMatrixVectors, svtkArrayDataAlgorithm);
  void PrintSelf(ostream& os, svtkIndent indent) override;

  //@{
  /**
   * Controls whether to normalize row-vectors or column-vectors.  0 = rows, 1 = columns.
   */
  svtkGetMacro(VectorDimension, int);
  svtkSetMacro(VectorDimension, int);
  //@}

  //@{
  /**
   * Value of p in p-norm normalization, subject to p >= 1.  Default is p=2 (Euclidean norm).
   */
  svtkGetMacro(PValue, double);
  svtkSetMacro(PValue, double);
  //@}

protected:
  svtkNormalizeMatrixVectors();
  ~svtkNormalizeMatrixVectors() override;

  int RequestData(svtkInformation*, svtkInformationVector**, svtkInformationVector*) override;

  int VectorDimension;
  double PValue;

private:
  svtkNormalizeMatrixVectors(const svtkNormalizeMatrixVectors&) = delete;
  void operator=(const svtkNormalizeMatrixVectors&) = delete;
};

#endif
