/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkTransposeMatrix.h

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
 * @class   svtkTransposeMatrix
 * @brief   Computes the transpose of an input matrix.
 *
 * @par Thanks:
 * Developed by Timothy M. Shead (tshead@sandia.gov) at Sandia National Laboratories.
 */

#ifndef svtkTransposeMatrix_h
#define svtkTransposeMatrix_h

#include "svtkArrayDataAlgorithm.h"
#include "svtkInfovisCoreModule.h" // For export macro

class SVTKINFOVISCORE_EXPORT svtkTransposeMatrix : public svtkArrayDataAlgorithm
{
public:
  static svtkTransposeMatrix* New();
  svtkTypeMacro(svtkTransposeMatrix, svtkArrayDataAlgorithm);
  void PrintSelf(ostream& os, svtkIndent indent) override;

protected:
  svtkTransposeMatrix();
  ~svtkTransposeMatrix() override;

  int RequestData(svtkInformation*, svtkInformationVector**, svtkInformationVector*) override;

private:
  svtkTransposeMatrix(const svtkTransposeMatrix&) = delete;
  void operator=(const svtkTransposeMatrix&) = delete;
};

#endif
