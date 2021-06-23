/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkEdgeLayout.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/*----------------------------------------------------------------------------
 Copyright (c) Sandia Corporation
 See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.
----------------------------------------------------------------------------*/
/**
 * @class   svtkEdgeLayout
 * @brief   layout graph edges
 *
 *
 * This class is a shell for many edge layout strategies which may be set
 * using the SetLayoutStrategy() function.  The layout strategies do the
 * actual work.
 */

#ifndef svtkEdgeLayout_h
#define svtkEdgeLayout_h

#include "svtkGraphAlgorithm.h"
#include "svtkInfovisLayoutModule.h" // For export macro

class svtkEdgeLayoutStrategy;
class svtkEventForwarderCommand;

class SVTKINFOVISLAYOUT_EXPORT svtkEdgeLayout : public svtkGraphAlgorithm
{
public:
  static svtkEdgeLayout* New();
  svtkTypeMacro(svtkEdgeLayout, svtkGraphAlgorithm);
  void PrintSelf(ostream& os, svtkIndent indent) override;

  //@{
  /**
   * The layout strategy to use during graph layout.
   */
  void SetLayoutStrategy(svtkEdgeLayoutStrategy* strategy);
  svtkGetObjectMacro(LayoutStrategy, svtkEdgeLayoutStrategy);
  //@}

  /**
   * Get the modification time of the layout algorithm.
   */
  svtkMTimeType GetMTime() override;

protected:
  svtkEdgeLayout();
  ~svtkEdgeLayout() override;

  svtkEdgeLayoutStrategy* LayoutStrategy;

  //@{
  /**
   * This intercepts events from the strategy object and re-emits them
   * as if they came from the layout engine itself.
   */
  svtkEventForwarderCommand* EventForwarder;
  unsigned long ObserverTag;
  //@}

  int RequestData(svtkInformation*, svtkInformationVector**, svtkInformationVector*) override;

private:
  svtkGraph* InternalGraph;

  svtkEdgeLayout(const svtkEdgeLayout&) = delete;
  void operator=(const svtkEdgeLayout&) = delete;
};

#endif
