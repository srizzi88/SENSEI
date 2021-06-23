/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkCollapseGraph.h

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
 * @class   svtkCollapseGraph
 * @brief   "Collapses" vertices onto their neighbors.
 *
 *
 * svtkCollapseGraph "collapses" vertices onto their neighbors, while maintaining
 * connectivity.  Two inputs are required - a graph (directed or undirected),
 * and a vertex selection that can be converted to indices.
 *
 * Conceptually, each of the vertices specified in the input selection
 * expands, "swallowing" adacent vertices.  Edges to-or-from the "swallowed"
 * vertices become edges to-or-from the expanding vertices, maintaining the
 * overall graph connectivity.
 *
 * In the case of directed graphs, expanding vertices only swallow vertices that
 * are connected via out edges.  This rule provides intuitive behavior when
 * working with trees, so that "child" vertices collapse into their parents
 * when the parents are part of the input selection.
 *
 * Input port 0: graph
 * Input port 1: selection
 */

#ifndef svtkCollapseGraph_h
#define svtkCollapseGraph_h

#include "svtkGraphAlgorithm.h"
#include "svtkInfovisCoreModule.h" // For export macro

class SVTKINFOVISCORE_EXPORT svtkCollapseGraph : public svtkGraphAlgorithm
{
public:
  static svtkCollapseGraph* New();
  svtkTypeMacro(svtkCollapseGraph, svtkGraphAlgorithm);
  void PrintSelf(ostream& os, svtkIndent indent) override;

  /// Convenience function provided for setting the graph input.
  void SetGraphConnection(svtkAlgorithmOutput*);
  /// Convenience function provided for setting the selection input.
  void SetSelectionConnection(svtkAlgorithmOutput*);

protected:
  svtkCollapseGraph();
  ~svtkCollapseGraph() override;

  int FillInputPortInformation(int port, svtkInformation* info) override;

  int RequestData(svtkInformation*, svtkInformationVector**, svtkInformationVector*) override;

private:
  svtkCollapseGraph(const svtkCollapseGraph&) = delete;
  void operator=(const svtkCollapseGraph&) = delete;
};

#endif
