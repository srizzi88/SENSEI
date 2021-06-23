/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkExtractSelectedGraph.h

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
 * @class   svtkExtractSelectedGraph
 * @brief   return a subgraph of a svtkGraph
 *
 *
 * The first input is a svtkGraph to take a subgraph from.
 * The second input (optional) is a svtkSelection containing selected
 * indices. The third input (optional) is a svtkAnnotationsLayers whose
 * annotations contain selected specifying selected indices.
 * The svtkSelection may have FIELD_TYPE set to POINTS (a vertex selection)
 * or CELLS (an edge selection).  A vertex selection preserves all edges
 * that connect selected vertices.  An edge selection preserves all vertices
 * that are adjacent to at least one selected edge.  Alternately, you may
 * indicate that an edge selection should maintain the full set of vertices,
 * by turning RemoveIsolatedVertices off.
 */

#ifndef svtkExtractSelectedGraph_h
#define svtkExtractSelectedGraph_h

#include "svtkGraphAlgorithm.h"
#include "svtkInfovisCoreModule.h" // For export macro

class svtkSelection;
class svtkDataSet;

class SVTKINFOVISCORE_EXPORT svtkExtractSelectedGraph : public svtkGraphAlgorithm
{
public:
  static svtkExtractSelectedGraph* New();
  svtkTypeMacro(svtkExtractSelectedGraph, svtkGraphAlgorithm);
  void PrintSelf(ostream& os, svtkIndent indent) override;

  /**
   * A convenience method for setting the second input (i.e. the selection).
   */
  void SetSelectionConnection(svtkAlgorithmOutput* in);

  /**
   * A convenience method for setting the third input (i.e. the annotation layers).
   */
  void SetAnnotationLayersConnection(svtkAlgorithmOutput* in);

  //@{
  /**
   * If set, removes vertices with no adjacent edges in an edge selection.
   * A vertex selection ignores this flag and always returns the full set
   * of selected vertices.  Default is on.
   */
  svtkSetMacro(RemoveIsolatedVertices, bool);
  svtkGetMacro(RemoveIsolatedVertices, bool);
  svtkBooleanMacro(RemoveIsolatedVertices, bool);
  //@}

  /**
   * Specify the first svtkGraph input and the second svtkSelection input.
   */
  int FillInputPortInformation(int port, svtkInformation* info) override;

protected:
  svtkExtractSelectedGraph();
  ~svtkExtractSelectedGraph() override;

  int RequestData(svtkInformation*, svtkInformationVector**, svtkInformationVector*) override;

  int RequestDataObject(svtkInformation*, svtkInformationVector**, svtkInformationVector*) override;

  bool RemoveIsolatedVertices;

private:
  svtkExtractSelectedGraph(const svtkExtractSelectedGraph&) = delete;
  void operator=(const svtkExtractSelectedGraph&) = delete;
};

#endif
