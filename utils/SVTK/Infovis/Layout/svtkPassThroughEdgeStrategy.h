/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkPassThroughEdgeStrategy.h

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
 * @class   svtkPassThroughEdgeStrategy
 * @brief   passes edge routing information through
 *
 *
 * Simply passes existing edge layout information from the input to the
 * output without making changes.
 */

#ifndef svtkPassThroughEdgeStrategy_h
#define svtkPassThroughEdgeStrategy_h

#include "svtkEdgeLayoutStrategy.h"
#include "svtkInfovisLayoutModule.h" // For export macro

class SVTKINFOVISLAYOUT_EXPORT svtkPassThroughEdgeStrategy : public svtkEdgeLayoutStrategy
{
public:
  static svtkPassThroughEdgeStrategy* New();
  svtkTypeMacro(svtkPassThroughEdgeStrategy, svtkEdgeLayoutStrategy);
  void PrintSelf(ostream& os, svtkIndent indent) override;

  /**
   * This is the layout method where the graph that was
   * set in SetGraph() is laid out.
   */
  void Layout() override;

protected:
  svtkPassThroughEdgeStrategy();
  ~svtkPassThroughEdgeStrategy() override;

private:
  svtkPassThroughEdgeStrategy(const svtkPassThroughEdgeStrategy&) = delete;
  void operator=(const svtkPassThroughEdgeStrategy&) = delete;
};

#endif
