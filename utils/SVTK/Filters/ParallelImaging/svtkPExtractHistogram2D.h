/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkPExtractHistogram2D.h

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
 * @class   svtkPExtractHistogram2D
 * @brief   compute a 2D histogram between two columns
 *  of an input svtkTable in parallel.
 *
 *
 *  This class does exactly the same this as svtkExtractHistogram2D,
 *  but does it in a multi-process environment.  After each node
 *  computes their own local histograms, this class does an AllReduce
 *  that distributes the sum of all local histograms onto each node.
 *
 * @sa
 *  svtkExtractHistogram2D
 *
 * @par Thanks:
 *  Developed by David Feng and Philippe Pebay at Sandia National Laboratories
 *------------------------------------------------------------------------------
 */

#ifndef svtkPExtractHistogram2D_h
#define svtkPExtractHistogram2D_h

#include "svtkExtractHistogram2D.h"
#include "svtkFiltersParallelImagingModule.h" // For export macro

class svtkMultiBlockDataSet;
class svtkMultiProcessController;

class SVTKFILTERSPARALLELIMAGING_EXPORT svtkPExtractHistogram2D : public svtkExtractHistogram2D
{
public:
  static svtkPExtractHistogram2D* New();
  svtkTypeMacro(svtkPExtractHistogram2D, svtkExtractHistogram2D);
  void PrintSelf(ostream& os, svtkIndent indent) override;

  virtual void SetController(svtkMultiProcessController*);
  svtkGetObjectMacro(Controller, svtkMultiProcessController);

protected:
  svtkPExtractHistogram2D();
  ~svtkPExtractHistogram2D() override;

  svtkMultiProcessController* Controller;

  int ComputeBinExtents(svtkDataArray* col1, svtkDataArray* col2) override;

  // Execute the calculations required by the Learn option.
  void Learn(svtkTable* inData, svtkTable* inParameters, svtkMultiBlockDataSet* outMeta) override;

private:
  svtkPExtractHistogram2D(const svtkPExtractHistogram2D&) = delete;
  void operator=(const svtkPExtractHistogram2D&) = delete;
};

#endif
