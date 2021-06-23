/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkPPairwiseExtractHistogram2D.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/*-------------------------------------------------------------------------
  Copyright 2009 Sandia Corporation.
  Under the terms of Contract DE-AC04-94AL85000 with Sandia Corporation,
  the U.S. Government retains certain rights in this software.
-------------------------------------------------------------------------*/
/**
 * @class   svtkPPairwiseExtractHistogram2D
 * @brief   compute a 2D histogram between
 *  all adjacent columns of an input svtkTable in parallel.
 *
 *
 *  This class does exactly the same this as svtkPairwiseExtractHistogram2D,
 *  but does it in a multi-process environment.  After each node
 *  computes their own local histograms, this class does an AllReduce
 *  that distributes the sum of all local histograms onto each node.
 *
 *  Because svtkPairwiseExtractHistogram2D is a light wrapper around a series
 *  of svtkExtractHistogram2D classes, this class just overrides the function
 *  that instantiates new histogram filters and returns the parallel version
 *  (svtkPExtractHistogram2D).
 *
 * @sa
 *  svtkExtractHistogram2D svtkPairwiseExtractHistogram2D svtkPExtractHistogram2D
 *
 * @par Thanks:
 *  Developed by David Feng and Philippe Pebay at Sandia National Laboratories
 *------------------------------------------------------------------------------
 */

#ifndef svtkPPairwiseExtractHistogram2D_h
#define svtkPPairwiseExtractHistogram2D_h

#include "svtkFiltersParallelImagingModule.h" // For export macro
#include "svtkPairwiseExtractHistogram2D.h"

class svtkExtractHistogram2D;
class svtkMultiProcessController;

class SVTKFILTERSPARALLELIMAGING_EXPORT svtkPPairwiseExtractHistogram2D
  : public svtkPairwiseExtractHistogram2D
{
public:
  static svtkPPairwiseExtractHistogram2D* New();
  svtkTypeMacro(svtkPPairwiseExtractHistogram2D, svtkPairwiseExtractHistogram2D);
  void PrintSelf(ostream& os, svtkIndent indent) override;

  virtual void SetController(svtkMultiProcessController*);
  svtkGetObjectMacro(Controller, svtkMultiProcessController);

protected:
  svtkPPairwiseExtractHistogram2D();
  ~svtkPPairwiseExtractHistogram2D() override;

  svtkMultiProcessController* Controller;

  /**
   * Generate a new histogram filter, but actually generate a parallel one this time.
   */
  svtkExtractHistogram2D* NewHistogramFilter() override;

private:
  svtkPPairwiseExtractHistogram2D(const svtkPPairwiseExtractHistogram2D&) = delete;
  void operator=(const svtkPPairwiseExtractHistogram2D&) = delete;
};

#endif
