/*=========================================================================

Program:   Visualization Toolkit
Module:    svtkPBivariateLinearTableThreshold.h

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
 * @class   svtkPBivariateLinearTableThreshold
 * @brief   performs line-based thresholding
 * for svtkTable data in parallel.
 *
 *
 * Perform the table filtering operations provided by
 * svtkBivariateLinearTableThreshold in parallel.
 */

#ifndef svtkPBivariateLinearTableThreshold_h
#define svtkPBivariateLinearTableThreshold_h

#include "svtkBivariateLinearTableThreshold.h"
#include "svtkFiltersParallelStatisticsModule.h" // For export macro

class svtkIdTypeArray;
class svtkMultiProcessController;

class SVTKFILTERSPARALLELSTATISTICS_EXPORT svtkPBivariateLinearTableThreshold
  : public svtkBivariateLinearTableThreshold
{
public:
  static svtkPBivariateLinearTableThreshold* New();
  svtkTypeMacro(svtkPBivariateLinearTableThreshold, svtkBivariateLinearTableThreshold);
  void PrintSelf(ostream& os, svtkIndent indent) override;

  //@{
  /**
   * Set the svtkMultiProcessController to be used for combining filter
   * results from the individual nodes.
   */
  virtual void SetController(svtkMultiProcessController*);
  svtkGetObjectMacro(Controller, svtkMultiProcessController);
  //@}

protected:
  svtkPBivariateLinearTableThreshold();
  ~svtkPBivariateLinearTableThreshold() override;

  int RequestData(svtkInformation*, svtkInformationVector**, svtkInformationVector*) override;

  svtkMultiProcessController* Controller;

private:
  svtkPBivariateLinearTableThreshold(const svtkPBivariateLinearTableThreshold&) = delete;
  void operator=(const svtkPBivariateLinearTableThreshold&) = delete;
};

#endif
