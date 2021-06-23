/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkArrayNorm.h

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
 * @class   svtkArrayNorm
 * @brief   Computes L-norms along one dimension of an array.
 *
 *
 * Given an input matrix (svtkTypedArray<double>), computes the L-norm for each
 * vector along either dimension, storing the results in a dense output
 * vector (1D svtkDenseArray<double>).  The caller may optionally request the
 * inverse norm as output (useful for subsequent normalization), and may limit
 * the computation to a "window" of vector elements, to avoid data copying.
 *
 * @par Thanks:
 * Developed by Timothy M. Shead (tshead@sandia.gov) at Sandia National Laboratories.
 */

#ifndef svtkArrayNorm_h
#define svtkArrayNorm_h

#include "svtkArrayDataAlgorithm.h"
#include "svtkArrayRange.h"
#include "svtkInfovisCoreModule.h" // For export macro

class SVTKINFOVISCORE_EXPORT svtkArrayNorm : public svtkArrayDataAlgorithm
{
public:
  static svtkArrayNorm* New();
  svtkTypeMacro(svtkArrayNorm, svtkArrayDataAlgorithm);
  void PrintSelf(ostream& os, svtkIndent indent) override;

  //@{
  /**
   * Controls the dimension along which norms will be computed.  For input matrices,
   * For input matrices, use "0" (rows) or "1" (columns). Default: 0
   */
  svtkGetMacro(Dimension, int);
  svtkSetMacro(Dimension, int);
  //@}

  //@{
  /**
   * Controls the L-value.  Default: 2
   */
  svtkGetMacro(L, int);
  void SetL(int value);
  //@}

  //@{
  /**
   * Controls whether to invert output values.  Default: false
   */
  svtkSetMacro(Invert, int);
  svtkGetMacro(Invert, int);
  //@}

  //@{
  /**
   * Defines an optional "window" used to compute the norm on a subset of the elements
   * in a vector.
   */
  void SetWindow(const svtkArrayRange& window);
  svtkArrayRange GetWindow();
  //@}

protected:
  svtkArrayNorm();
  ~svtkArrayNorm() override;

  int RequestData(svtkInformation*, svtkInformationVector**, svtkInformationVector*) override;

private:
  svtkArrayNorm(const svtkArrayNorm&) = delete;
  void operator=(const svtkArrayNorm&) = delete;

  int Dimension;
  int L;
  int Invert;
  svtkArrayRange Window;
};

#endif

// SVTK-HeaderTest-Exclude: svtkArrayNorm.h
