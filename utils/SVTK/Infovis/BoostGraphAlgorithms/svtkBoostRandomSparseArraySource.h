/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkBoostRandomSparseArraySource.h

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
 * @class   svtkBoostRandomSparseArraySource
 * @brief   generates a sparse N-way array containing random values.
 *
 * @par Thanks:
 * Developed by Timothy M. Shead (tshead@sandia.gov) at Sandia National Laboratories.
 */

#ifndef svtkBoostRandomSparseArraySource_h
#define svtkBoostRandomSparseArraySource_h

#include "svtkArrayDataAlgorithm.h"
#include "svtkArrayExtents.h"
#include "svtkInfovisBoostGraphAlgorithmsModule.h" // For export macro

class SVTKINFOVISBOOSTGRAPHALGORITHMS_EXPORT svtkBoostRandomSparseArraySource
  : public svtkArrayDataAlgorithm
{
public:
  static svtkBoostRandomSparseArraySource* New();
  svtkTypeMacro(svtkBoostRandomSparseArraySource, svtkArrayDataAlgorithm);
  void PrintSelf(ostream& os, svtkIndent indent) override;

  /**
   * Sets the extents (dimensionality and size) of the output array
   */
  void SetExtents(const svtkArrayExtents&);

  /**
   * Returns the extents (dimensionality and size) of the output array
   */
  svtkArrayExtents GetExtents();

  //@{
  /**
   * Stores a random-number-seed for determining which elements within
   * the output matrix will have non-zero values
   */
  svtkGetMacro(ElementProbabilitySeed, svtkTypeUInt32);
  svtkSetMacro(ElementProbabilitySeed, svtkTypeUInt32);
  //@}

  //@{
  /**
   * Stores the probability (in the range [0, 1]) that an element within
   * the output matrix will have a non-zero value
   */
  svtkGetMacro(ElementProbability, double);
  svtkSetMacro(ElementProbability, double);
  //@}

  //@{
  /**
   * Stores a random-number-seed for computing random element values
   */
  svtkGetMacro(ElementValueSeed, svtkTypeUInt32);
  svtkSetMacro(ElementValueSeed, svtkTypeUInt32);
  //@}

  //@{
  /**
   * Stores the minimum value of any element
   */
  svtkGetMacro(MinValue, double);
  svtkSetMacro(MinValue, double);
  //@}

  //@{
  /**
   * Stores the maximum value of any element
   */
  svtkGetMacro(MaxValue, double);
  svtkSetMacro(MaxValue, double);
  //@}

protected:
  svtkBoostRandomSparseArraySource();
  ~svtkBoostRandomSparseArraySource() override;

  int RequestData(svtkInformation*, svtkInformationVector**, svtkInformationVector*) override;

private:
  svtkBoostRandomSparseArraySource(const svtkBoostRandomSparseArraySource&) = delete;
  void operator=(const svtkBoostRandomSparseArraySource&) = delete;

  svtkArrayExtents Extents;

  svtkTypeUInt32 ElementProbabilitySeed;
  double ElementProbability;

  svtkTypeUInt32 ElementValueSeed;
  double MinValue;
  double MaxValue;
};

#endif

// SVTK-HeaderTest-Exclude: svtkBoostRandomSparseArraySource.h
