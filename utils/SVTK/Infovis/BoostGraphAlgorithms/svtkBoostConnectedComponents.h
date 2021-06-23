/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkBoostConnectedComponents.h

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
 * @class   svtkBoostConnectedComponents
 * @brief   Find the connected components of a graph
 *
 *
 * svtkBoostConnectedComponents discovers the connected regions of a svtkGraph.
 * Each vertex is assigned a component ID in the vertex array "component".
 * If the graph is undirected, this is the natural connected components
 * of the graph.  If the graph is directed, this filter discovers the
 * strongly connected components of the graph (i.e. the maximal sets of
 * vertices where there is a directed path between any pair of vertices
 * within each set).
 */

#ifndef svtkBoostConnectedComponents_h
#define svtkBoostConnectedComponents_h

#include "svtkGraphAlgorithm.h"
#include "svtkInfovisBoostGraphAlgorithmsModule.h" // For export macro

class SVTKINFOVISBOOSTGRAPHALGORITHMS_EXPORT svtkBoostConnectedComponents : public svtkGraphAlgorithm
{
public:
  static svtkBoostConnectedComponents* New();
  svtkTypeMacro(svtkBoostConnectedComponents, svtkGraphAlgorithm);
  void PrintSelf(ostream& os, svtkIndent indent) override;

protected:
  svtkBoostConnectedComponents();
  ~svtkBoostConnectedComponents() override;

  int RequestData(svtkInformation*, svtkInformationVector**, svtkInformationVector*) override;

private:
  svtkBoostConnectedComponents(const svtkBoostConnectedComponents&) = delete;
  void operator=(const svtkBoostConnectedComponents&) = delete;
};

#endif
