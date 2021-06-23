/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkGraphLayout.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/*-------------------------------------------------------------------------
  Copyright 2008 Sandia Corporation.
  Under the terms of Contract DE-AC04-94AL85000 with Sandia Corporation,
  the U.S. Government retains certain rights in this software.
-------------------------------------------------------------------------*/
/**
 * @class   svtkGraphLayout
 * @brief   layout a graph in 2 or 3 dimensions
 *
 *
 * This class is a shell for many graph layout strategies which may be set
 * using the SetLayoutStrategy() function.  The layout strategies do the
 * actual work.
 *
 * .SECTION Thanks
 * Thanks to Brian Wylie from Sandia National Laboratories for adding incremental
 * layout capabilities.
 */

#ifndef svtkGraphLayout_h
#define svtkGraphLayout_h

#include "svtkGraphAlgorithm.h"
#include "svtkInfovisLayoutModule.h" // For export macro

class svtkAbstractTransform;
class svtkEventForwarderCommand;
class svtkGraphLayoutStrategy;

class SVTKINFOVISLAYOUT_EXPORT svtkGraphLayout : public svtkGraphAlgorithm
{
public:
  static svtkGraphLayout* New();
  svtkTypeMacro(svtkGraphLayout, svtkGraphAlgorithm);
  void PrintSelf(ostream& os, svtkIndent indent) override;

  //@{
  /**
   * The layout strategy to use during graph layout.
   */
  void SetLayoutStrategy(svtkGraphLayoutStrategy* strategy);
  svtkGetObjectMacro(LayoutStrategy, svtkGraphLayoutStrategy);
  //@}

  /**
   * Ask the layout algorithm if the layout is complete
   */
  virtual int IsLayoutComplete();

  /**
   * Get the modification time of the layout algorithm.
   */
  svtkMTimeType GetMTime() override;

  //@{
  /**
   * Set the ZRange for the output data.
   * If the initial layout is planar (i.e. all z coordinates are zero),
   * the coordinates will be evenly spaced from 0.0 to ZRange.
   * The default is zero, which has no effect.
   */
  svtkGetMacro(ZRange, double);
  svtkSetMacro(ZRange, double);
  //@}

  //@{
  /**
   * Transform the graph vertices after the layout.
   */
  svtkGetObjectMacro(Transform, svtkAbstractTransform);
  virtual void SetTransform(svtkAbstractTransform* t);
  //@}

  //@{
  /**
   * Whether to use the specified transform after layout.
   */
  svtkSetMacro(UseTransform, bool);
  svtkGetMacro(UseTransform, bool);
  svtkBooleanMacro(UseTransform, bool);
  //@}

protected:
  svtkGraphLayout();
  ~svtkGraphLayout() override;

  svtkGraphLayoutStrategy* LayoutStrategy;

  /**
   * This intercepts events from the strategy object and re-emits them
   * as if they came from the layout engine itself.
   */
  svtkEventForwarderCommand* EventForwarder;

  int RequestData(svtkInformation*, svtkInformationVector**, svtkInformationVector*) override;

private:
  svtkGraph* LastInput;
  svtkGraph* InternalGraph;
  svtkMTimeType LastInputMTime;
  bool StrategyChanged;
  double ZRange;
  svtkAbstractTransform* Transform;
  bool UseTransform;

  svtkGraphLayout(const svtkGraphLayout&) = delete;
  void operator=(const svtkGraphLayout&) = delete;
};

#endif
