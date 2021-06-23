/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkEdgeLayoutStrategy.h

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
 * @class   svtkEdgeLayoutStrategy
 * @brief   abstract superclass for all edge layout strategies
 *
 *
 * All edge layouts should subclass from this class.  svtkEdgeLayoutStrategy
 * works as a plug-in to the svtkEdgeLayout algorithm.
 */

#ifndef svtkEdgeLayoutStrategy_h
#define svtkEdgeLayoutStrategy_h

#include "svtkInfovisLayoutModule.h" // For export macro
#include "svtkObject.h"

class svtkGraph;

class SVTKINFOVISLAYOUT_EXPORT svtkEdgeLayoutStrategy : public svtkObject
{
public:
  svtkTypeMacro(svtkEdgeLayoutStrategy, svtkObject);
  void PrintSelf(ostream& os, svtkIndent indent) override;

  /**
   * Setting the graph for the layout strategy
   */
  virtual void SetGraph(svtkGraph* graph);

  /**
   * This method allows the layout strategy to
   * do initialization of data structures
   * or whatever else it might want to do.
   */
  virtual void Initialize() {}

  /**
   * This is the layout method where the graph that was
   * set in SetGraph() is laid out.
   */
  virtual void Layout() = 0;

  //@{
  /**
   * Set/Get the field to use for the edge weights.
   */
  svtkSetStringMacro(EdgeWeightArrayName);
  svtkGetStringMacro(EdgeWeightArrayName);
  //@}

protected:
  svtkEdgeLayoutStrategy();
  ~svtkEdgeLayoutStrategy() override;

  svtkGraph* Graph;
  char* EdgeWeightArrayName;

private:
  svtkEdgeLayoutStrategy(const svtkEdgeLayoutStrategy&) = delete;
  void operator=(const svtkEdgeLayoutStrategy&) = delete;
};

#endif
