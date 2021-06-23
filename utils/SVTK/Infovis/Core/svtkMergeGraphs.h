/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkMergeGraphs.h

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
 * @class   svtkMergeGraphs
 * @brief   combines two graphs
 *
 *
 * svtkMergeGraphs combines information from two graphs into one.
 * Both graphs must have pedigree ids assigned to the vertices.
 * The output will contain the vertices/edges in the first graph, in
 * addition to:
 *
 *  - vertices in the second graph whose pedigree id does not
 *    match a vertex in the first input
 *
 *  - edges in the second graph
 *
 * The output will contain the same attribute structure as the input;
 * fields associated only with the second input graph will not be passed
 * to the output. When possible, the vertex/edge data for new vertices and
 * edges will be populated with matching attributes on the second graph.
 * To be considered a matching attribute, the array must have the same name,
 * type, and number of components.
 *
 * @warning
 * This filter is not "domain-aware". Pedigree ids are assumed to be globally
 * unique, regardless of their domain.
 */

#ifndef svtkMergeGraphs_h
#define svtkMergeGraphs_h

#include "svtkGraphAlgorithm.h"
#include "svtkInfovisCoreModule.h" // For export macro

class svtkBitArray;
class svtkMutableGraphHelper;
class svtkStringArray;
class svtkTable;

class SVTKINFOVISCORE_EXPORT svtkMergeGraphs : public svtkGraphAlgorithm
{
public:
  static svtkMergeGraphs* New();
  svtkTypeMacro(svtkMergeGraphs, svtkGraphAlgorithm);
  void PrintSelf(ostream& os, svtkIndent indent) override;

  /**
   * This is the core functionality of the algorithm. Adds edges
   * and vertices from g2 into g1.
   */
  int ExtendGraph(svtkMutableGraphHelper* g1, svtkGraph* g2);

  //@{
  /**
   * Whether to use an edge window array. The default is to
   * not use a window array.
   */
  svtkSetMacro(UseEdgeWindow, bool);
  svtkGetMacro(UseEdgeWindow, bool);
  svtkBooleanMacro(UseEdgeWindow, bool);
  //@}

  //@{
  /**
   * The edge window array. The default array name is "time".
   */
  svtkSetStringMacro(EdgeWindowArrayName);
  svtkGetStringMacro(EdgeWindowArrayName);
  //@}

  //@{
  /**
   * The time window amount. Edges with values lower
   * than the maximum value minus this window will be
   * removed from the graph. The default edge window is
   * 10000.
   */
  svtkSetMacro(EdgeWindow, double);
  svtkGetMacro(EdgeWindow, double);
  //@}

protected:
  svtkMergeGraphs();
  ~svtkMergeGraphs() override;

  int RequestData(svtkInformation*, svtkInformationVector**, svtkInformationVector*) override;

  int FillInputPortInformation(int port, svtkInformation* info) override;

  bool UseEdgeWindow;
  char* EdgeWindowArrayName;
  double EdgeWindow;

private:
  svtkMergeGraphs(const svtkMergeGraphs&) = delete;
  void operator=(const svtkMergeGraphs&) = delete;
};

#endif
