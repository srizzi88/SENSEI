/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkTreeDifferenceFilter.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

/**
 * @class   svtkTreeDifferenceFilter
 * @brief   compare two trees
 *
 *
 * svtkTreeDifferenceFilter compares two trees by analyzing a svtkDoubleArray.
 * Each tree must have a copy of this array.  A user of this filter should
 * call SetComparisonArrayName to specify the array that should be used as
 * the basis of coparison.  This array can either be part of the trees'
 * EdgeData or VertexData.
 *
 */

#ifndef svtkTreeDifferenceFilter_h
#define svtkTreeDifferenceFilter_h

#include "svtkGraphAlgorithm.h"
#include "svtkInfovisCoreModule.h" // For export macro

#include "svtkSmartPointer.h" // For ivars
#include <vector>            // For ivars

class svtkDoubleArray;
class svtkTree;

class SVTKINFOVISCORE_EXPORT svtkTreeDifferenceFilter : public svtkGraphAlgorithm
{
public:
  static svtkTreeDifferenceFilter* New();
  svtkTypeMacro(svtkTreeDifferenceFilter, svtkGraphAlgorithm);
  void PrintSelf(ostream& os, svtkIndent indent) override;

  //@{
  /**
   * Set/Get the name of the identifier array in the trees' VertexData.
   * This array is used to find corresponding vertices in the two trees.
   * If this array name is not set, then we assume that the vertices in
   * the two trees to compare have corresponding svtkIdTypes.
   * Otherwise, the named array must be a svtkStringArray.
   * The identifier array does not necessarily have to specify a name for
   * each vertex in the tree.  If some vertices are unnamed, then this
   * filter will assign correspondence between ancestors of named vertices.
   */
  svtkSetStringMacro(IdArrayName);
  svtkGetStringMacro(IdArrayName);
  //@}

  //@{
  /**
   * Set/Get the name of the array that we're comparing between the two trees.
   * The named array must be a svtkDoubleArray.
   */
  svtkSetStringMacro(ComparisonArrayName);
  svtkGetStringMacro(ComparisonArrayName);
  //@}

  //@{
  /**
   * Set/Get the name of a new svtkDoubleArray that will contain the results of
   * the comparison between the two trees.  This new array will be added to
   * the input tree's VertexData or EdgeData, based on the value of
   * ComparisonArrayIsVertexData.  If this method is not called, the new
   * svtkDoubleArray will be named "difference" by default.
   */
  svtkSetStringMacro(OutputArrayName);
  svtkGetStringMacro(OutputArrayName);
  //@}

  //@{
  /**
   * Specify whether the comparison array is within the trees' vertex data or
   * not.  By default, we assume that the array to compare is within the trees'
   * EdgeData().
   */
  svtkSetMacro(ComparisonArrayIsVertexData, bool);
  svtkGetMacro(ComparisonArrayIsVertexData, bool);
  //@}

protected:
  svtkTreeDifferenceFilter();
  ~svtkTreeDifferenceFilter() override;

  int RequestData(svtkInformation*, svtkInformationVector**, svtkInformationVector*) override;

  int FillInputPortInformation(int port, svtkInformation* info) override;

  /**
   * Populate VertexMap and EdgeMap with meaningful values.  These maps
   * allow us to look up the svtkIdType of a vertex or edge in tree #2,
   * given its svtkIdType in tree #1.
   */
  bool GenerateMapping(svtkTree* tree1, svtkTree* tree2);

  /**
   * Compute the differences between tree #1 and tree #2's copies of the
   * comparison array.
   */
  svtkSmartPointer<svtkDoubleArray> ComputeDifference(svtkTree* tree1, svtkTree* tree2);

  char* IdArrayName;
  char* ComparisonArrayName;
  char* OutputArrayName;
  bool ComparisonArrayIsVertexData;

  std::vector<svtkIdType> VertexMap;
  std::vector<svtkIdType> EdgeMap;

private:
  svtkTreeDifferenceFilter(const svtkTreeDifferenceFilter&) = delete;
  void operator=(const svtkTreeDifferenceFilter&) = delete;
};

#endif
