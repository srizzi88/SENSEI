/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkBoostBreadthFirstSearchTree.h

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
 * @class   svtkBoostBreadthFirstSearchTree
 * @brief   Constructs a BFS tree from a graph
 *
 *
 *
 * This svtk class uses the Boost breadth_first_search
 * generic algorithm to perform a breadth first search from a given
 * a 'source' vertex on the input graph (a svtkGraph).
 * The result is a tree with root node corresponding to the start node
 * of the search.
 *
 * @sa
 * svtkGraph svtkBoostGraphAdapter
 */

#ifndef svtkBoostBreadthFirstSearchTree_h
#define svtkBoostBreadthFirstSearchTree_h

#include "svtkInfovisBoostGraphAlgorithmsModule.h" // For export macro
#include "svtkStdString.h"                         // For string type
#include "svtkVariant.h"                           // For variant type

#include "svtkTreeAlgorithm.h"

class SVTKINFOVISBOOSTGRAPHALGORITHMS_EXPORT svtkBoostBreadthFirstSearchTree : public svtkTreeAlgorithm
{
public:
  static svtkBoostBreadthFirstSearchTree* New();
  svtkTypeMacro(svtkBoostBreadthFirstSearchTree, svtkTreeAlgorithm);
  void PrintSelf(ostream& os, svtkIndent indent) override;

  /**
   * Set the index (into the vertex array) of the
   * breadth first search 'origin' vertex.
   */
  void SetOriginVertex(svtkIdType index);

  /**
   * Set the breadth first search 'origin' vertex.
   * This method is basically the same as above
   * but allows the application to simply specify
   * an array name and value, instead of having to
   * know the specific index of the vertex.
   */
  void SetOriginVertex(svtkStdString arrayName, svtkVariant value);

  //@{
  /**
   * Stores the graph vertex ids for the tree vertices in an array
   * named "GraphVertexId".  Default is off.
   */
  svtkSetMacro(CreateGraphVertexIdArray, bool);
  svtkGetMacro(CreateGraphVertexIdArray, bool);
  svtkBooleanMacro(CreateGraphVertexIdArray, bool);
  //@}

  //@{
  /**
   * Turn on this option to reverse the edges in the graph.
   */
  svtkSetMacro(ReverseEdges, bool);
  svtkGetMacro(ReverseEdges, bool);
  svtkBooleanMacro(ReverseEdges, bool);
  //@}

protected:
  svtkBoostBreadthFirstSearchTree();
  ~svtkBoostBreadthFirstSearchTree() override;

  int FillInputPortInformation(int port, svtkInformation* info) override;

  int RequestData(svtkInformation*, svtkInformationVector**, svtkInformationVector*) override;

private:
  svtkIdType OriginVertexIndex;
  char* ArrayName;
  svtkVariant OriginValue;
  bool ArrayNameSet;
  bool CreateGraphVertexIdArray;
  bool ReverseEdges;

  //@{
  /**
   * Using the convenience function for set strings internally
   */
  svtkSetStringMacro(ArrayName);
  //@}

  /**
   * This method is basically a helper function to find
   * the index of a specific value within a specific array
   */
  svtkIdType GetVertexIndex(svtkAbstractArray* abstract, svtkVariant value);

  svtkBoostBreadthFirstSearchTree(const svtkBoostBreadthFirstSearchTree&) = delete;
  void operator=(const svtkBoostBreadthFirstSearchTree&) = delete;
};

#endif
