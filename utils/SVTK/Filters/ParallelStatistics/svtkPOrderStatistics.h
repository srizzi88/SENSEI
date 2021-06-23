/*=========================================================================

Program:   Visualization Toolkit
Module:    svtkPOrderStatistics.h

Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
All rights reserved.
See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

This software is distributed WITHOUT ANY WARRANTY; without even
the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/*-------------------------------------------------------------------------
  Copyright 2011 Sandia Corporation.
  Under the terms of Contract DE-AC04-94AL85000 with Sandia Corporation,
  the U.S. Government retains certain rights in this software.
  -------------------------------------------------------------------------*/
/**
 * @class   svtkPOrderStatistics
 * @brief   A class for parallel univariate order statistics
 *
 * svtkPOrderStatistics is svtkOrderStatistics subclass for parallel datasets.
 * It learns and derives the global statistical model on each node, but assesses each
 * individual data points on the node that owns it.
 *
 * .NOTE: It is assumed that the keys in the histogram table be contained in the set {0,...,n-1}
 * of successive integers, where n is the number of rows of the summary table.
 * If this requirement is not fulfilled, then the outcome of the parallel update of order
 * tables is unpredictable but will most likely be a crash.
 * Note that this requirement is consistent with the way histogram tables are constructed
 * by the (serial) superclass and thus, if you are using this class as it is intended to be ran,
 * then you do not have to worry about this requirement.
 *
 * @par Thanks:
 * Thanks to Philippe Pebay from Sandia National Laboratories for implementing this class.
 */

#ifndef svtkPOrderStatistics_h
#define svtkPOrderStatistics_h

#include "svtkFiltersParallelStatisticsModule.h" // For export macro
#include "svtkOrderStatistics.h"

#include <map> // STL Header

class svtkIdTypeArray;
class svtkMultiBlockDataSet;
class svtkMultiProcessController;

class SVTKFILTERSPARALLELSTATISTICS_EXPORT svtkPOrderStatistics : public svtkOrderStatistics
{
public:
  static svtkPOrderStatistics* New();
  svtkTypeMacro(svtkPOrderStatistics, svtkOrderStatistics);
  void PrintSelf(ostream& os, svtkIndent indent) override;

  //@{
  /**
   * Get/Set the multiprocess controller. If no controller is set,
   * single process is assumed.
   */
  virtual void SetController(svtkMultiProcessController*);
  svtkGetObjectMacro(Controller, svtkMultiProcessController);
  //@}

  /**
   * Execute the parallel calculations required by the Learn option.
   */
  void Learn(svtkTable*, svtkTable*, svtkMultiBlockDataSet*) override;

protected:
  svtkPOrderStatistics();
  ~svtkPOrderStatistics() override;

  /**
   * Reduce the collection of local histograms to the global one for data inputs
   */
  bool Reduce(svtkIdTypeArray*, svtkDataArray*);

  /**
   * Reduce the collection of local histograms to the global one for string inputs
   */
  bool Reduce(svtkIdTypeArray*, svtkIdType&, char*, std::map<svtkStdString, svtkIdType>&);

  /**
   * Broadcast reduced histogram to all processes in the case of string inputs
   */
  bool Broadcast(std::map<svtkStdString, svtkIdType>&, svtkIdTypeArray*, svtkStringArray*, svtkIdType);

  svtkMultiProcessController* Controller;

private:
  svtkPOrderStatistics(const svtkPOrderStatistics&) = delete;
  void operator=(const svtkPOrderStatistics&) = delete;
};

#endif
