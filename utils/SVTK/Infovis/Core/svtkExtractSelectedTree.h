/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkExtractSelectedTree.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   svtkExtractSelectedTree
 * @brief   return a subtree from a svtkTree
 *
 *
 * input 0 --- a svtkTree
 * input 1 --- a svtkSelection, containing selected vertices. It may have
 * FILED_type set to POINTS ( a vertex selection) or CELLS (an edge selection).
 * A vertex selection preserves the edges that connect selected vertices.
 * An edge selection perserves the vertices that are adjacent to at least one
 * selected edges.
 */

#ifndef svtkExtractSelectedTree_h
#define svtkExtractSelectedTree_h

#include "svtkInfovisCoreModule.h" // For export macro
#include "svtkTreeAlgorithm.h"

class svtkTree;
class svtkIdTypeArray;
class svtkMutableDirectedGraph;

class SVTKINFOVISCORE_EXPORT svtkExtractSelectedTree : public svtkTreeAlgorithm
{
public:
  static svtkExtractSelectedTree* New();
  svtkTypeMacro(svtkExtractSelectedTree, svtkTreeAlgorithm);
  void PrintSelf(ostream& os, svtkIndent indent) override;

  /**
   * A convenience method for setting the second input (i.e. the selection).
   */
  void SetSelectionConnection(svtkAlgorithmOutput* in);

  int FillInputPortInformation(int port, svtkInformation* info) override;

protected:
  svtkExtractSelectedTree();
  ~svtkExtractSelectedTree() override;

  int RequestData(svtkInformation*, svtkInformationVector**, svtkInformationVector*) override;

  int BuildTree(svtkTree* inputTree, svtkIdTypeArray* list, svtkMutableDirectedGraph* builder);

private:
  svtkExtractSelectedTree(const svtkExtractSelectedTree&) = delete;
  void operator=(const svtkExtractSelectedTree&) = delete;
};

#endif
