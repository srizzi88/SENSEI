/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkPComputeHistogram2DOutliers.h

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
 * @class   svtkPComputeHistogram2DOutliers
 * @brief   extract outlier rows from
 *  a svtkTable based on input 2D histograms, in parallel.
 *
 *
 *  This class does exactly the same this as svtkComputeHistogram2DOutliers,
 *  but does it in a multi-process environment.  After each node
 *  computes their own local outliers, class does an AllGather
 *  that distributes the outliers to every node.  This could probably just
 *  be a Gather onto the root node instead.
 *
 *  After this operation, the row selection will only contain local row ids,
 *  since I'm not sure how to deal with distributed ids.
 *
 * @sa
 *  svtkComputeHistogram2DOutliers
 *
 * @par Thanks:
 *  Developed by David Feng at Sandia National Laboratories
 *------------------------------------------------------------------------------
 */

#ifndef svtkPComputeHistogram2DOutliers_h
#define svtkPComputeHistogram2DOutliers_h
//------------------------------------------------------------------------------
#include "svtkComputeHistogram2DOutliers.h"
#include "svtkFiltersParallelImagingModule.h" // For export macro
//------------------------------------------------------------------------------
class svtkMultiProcessController;
//------------------------------------------------------------------------------
class SVTKFILTERSPARALLELIMAGING_EXPORT svtkPComputeHistogram2DOutliers
  : public svtkComputeHistogram2DOutliers
{
public:
  static svtkPComputeHistogram2DOutliers* New();
  svtkTypeMacro(svtkPComputeHistogram2DOutliers, svtkComputeHistogram2DOutliers);
  void PrintSelf(ostream& os, svtkIndent indent) override;

  virtual void SetController(svtkMultiProcessController*);
  svtkGetObjectMacro(Controller, svtkMultiProcessController);

protected:
  svtkPComputeHistogram2DOutliers();
  ~svtkPComputeHistogram2DOutliers() override;

  int RequestData(svtkInformation*, svtkInformationVector**, svtkInformationVector*) override;

  svtkMultiProcessController* Controller;

private:
  svtkPComputeHistogram2DOutliers(const svtkPComputeHistogram2DOutliers&) = delete;
  void operator=(const svtkPComputeHistogram2DOutliers&) = delete;
};

#endif
