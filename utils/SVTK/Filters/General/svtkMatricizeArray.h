/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkMatricizeArray.h

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
 * @class   svtkMatricizeArray
 * @brief   Convert an array of arbitrary dimensions to a
 * matrix.
 *
 *
 * Given a sparse input array of arbitrary dimension, creates a sparse output
 * matrix (svtkSparseArray<double>) where each column is a slice along an
 * arbitrary dimension from the source.
 *
 * @par Thanks:
 * Developed by Timothy M. Shead (tshead@sandia.gov) at Sandia National Laboratories.
 */

#ifndef svtkMatricizeArray_h
#define svtkMatricizeArray_h

#include "svtkArrayDataAlgorithm.h"
#include "svtkFiltersGeneralModule.h" // For export macro

class SVTKFILTERSGENERAL_EXPORT svtkMatricizeArray : public svtkArrayDataAlgorithm
{
public:
  static svtkMatricizeArray* New();
  svtkTypeMacro(svtkMatricizeArray, svtkArrayDataAlgorithm);
  void PrintSelf(ostream& os, svtkIndent indent) override;

  //@{
  /**
   * Returns the 0-numbered dimension that will be mapped to columns in the output
   */
  svtkGetMacro(SliceDimension, svtkIdType);
  //@}

  //@{
  /**
   * Sets the 0-numbered dimension that will be mapped to columns in the output
   */
  svtkSetMacro(SliceDimension, svtkIdType);
  //@}

protected:
  svtkMatricizeArray();
  ~svtkMatricizeArray() override;

  int RequestData(svtkInformation*, svtkInformationVector**, svtkInformationVector*) override;

private:
  svtkMatricizeArray(const svtkMatricizeArray&) = delete;
  void operator=(const svtkMatricizeArray&) = delete;

  class Generator;

  svtkIdType SliceDimension;
};

#endif
