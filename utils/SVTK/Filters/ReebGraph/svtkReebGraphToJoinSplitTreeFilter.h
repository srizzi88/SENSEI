/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkReebGraphToJoinSplitTreeFilter.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   svtkReebGraphToJoinSplitTreeFilter
 * @brief   converts a given Reeb graph
 * either to a join tree or a split tree (respectively the connectivity of the
 * sub- and sur- level sets).
 * Note: if you want to use simplification filters, do so on the input Reeb
 * graph first.
 *
 * Reference:
 * "Computing contpour trees in all dimensions". H. Carr, J. Snoeyink, U. Axen.
 * SODA 2000, pp. 918-926.
 *
 *
 * The filter takes as an input the underlying mesh (port 0, a svtkPolyData for
 * 2D meshes or a svtkUnstructuredGrid for 3D meshes) with an attached scalar
 * field (identified by FieldId, with setFieldId()) and an input Reeb graph
 * computed on that mesh (port 1).
 * The outputs is svtkReebGraph object describing either a join or split tree.
 */

#ifndef svtkReebGraphToJoinSplitTreeFilter_h
#define svtkReebGraphToJoinSplitTreeFilter_h

#include "svtkDirectedGraphAlgorithm.h"
#include "svtkFiltersReebGraphModule.h" // For export macro

class svtkReebGraph;

class SVTKFILTERSREEBGRAPH_EXPORT svtkReebGraphToJoinSplitTreeFilter
  : public svtkDirectedGraphAlgorithm
{
public:
  static svtkReebGraphToJoinSplitTreeFilter* New();
  svtkTypeMacro(svtkReebGraphToJoinSplitTreeFilter, svtkDirectedGraphAlgorithm);
  void PrintSelf(ostream& os, svtkIndent indent) override;

  //@{
  /**
   * Specify if you want to get a join or a split tree.
   * Default value: false (join tree)
   */
  svtkSetMacro(IsSplitTree, bool);
  svtkGetMacro(IsSplitTree, bool);
  //@}

  //@{
  /**
   * Set the scalar field Id
   * Default value: 0;
   */
  svtkSetMacro(FieldId, svtkIdType);
  svtkGetMacro(FieldId, svtkIdType);
  //@}

  svtkReebGraph* GetOutput();

protected:
  svtkReebGraphToJoinSplitTreeFilter();
  ~svtkReebGraphToJoinSplitTreeFilter() override;

  bool IsSplitTree;

  svtkIdType FieldId;

  int FillInputPortInformation(int portNumber, svtkInformation*) override;
  int FillOutputPortInformation(int, svtkInformation*) override;

  int RequestData(svtkInformation* request, svtkInformationVector** inputVector,
    svtkInformationVector* outputVector) override;

private:
  svtkReebGraphToJoinSplitTreeFilter(const svtkReebGraphToJoinSplitTreeFilter&) = delete;
  void operator=(const svtkReebGraphToJoinSplitTreeFilter&) = delete;
};

#endif
