/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkExtractBlock.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   svtkExtractBlock
 * @brief   extracts blocks from a multiblock dataset.
 *
 * svtkExtractBlock is a filter that extracts blocks from a multiblock
 * dataset.  Each node in the multi-block tree is identified by an \c
 * index. The index can be obtained by performing a preorder traversal of the
 * tree (including empty nodes). eg. A(B (D, E), C(F, G)).  Inorder traversal
 * yields: A, B, D, E, C, F, G Index of A is 0, while index of C is 4.
 *
 * Note that if you specify node 0, then the input is simply shallow copied
 * to the output. This is true even if other nodes are specified along with
 * node 0.
 */

#ifndef svtkExtractBlock_h
#define svtkExtractBlock_h

#include "svtkFiltersExtractionModule.h" // For export macro
#include "svtkMultiBlockDataSetAlgorithm.h"

class svtkDataObjectTreeIterator;
class svtkMultiPieceDataSet;

class SVTKFILTERSEXTRACTION_EXPORT svtkExtractBlock : public svtkMultiBlockDataSetAlgorithm
{
public:
  //@{
  /**
   * Standard methods for instantiation, type information, and printing.
   */
  static svtkExtractBlock* New();
  svtkTypeMacro(svtkExtractBlock, svtkMultiBlockDataSetAlgorithm);
  void PrintSelf(ostream& os, svtkIndent indent) override;
  //@{

  //@{
  /**
   * Select the block indices to extract.  Each node in the multi-block tree
   * is identified by an \c index. The index can be obtained by performing a
   * preorder traversal of the tree (including empty nodes). eg. A(B (D, E),
   * C(F, G)).  Inorder traversal yields: A, B, D, E, C, F, G Index of A is
   * 0, while index of C is 4. (Note: specifying node 0 means the input is
   * copied to the output.)
   */
  void AddIndex(unsigned int index);
  void RemoveIndex(unsigned int index);
  void RemoveAllIndices();
  //@}

  //@{
  /**
   * When set, the output multiblock dataset will be pruned to remove empty
   * nodes. On by default.
   */
  svtkSetMacro(PruneOutput, svtkTypeBool);
  svtkGetMacro(PruneOutput, svtkTypeBool);
  svtkBooleanMacro(PruneOutput, svtkTypeBool);
  //@}

  //@{
  /**
   * This is used only when PruneOutput is ON. By default, when pruning the
   * output i.e. remove empty blocks, if node has only 1 non-null child block,
   * then that node is removed. To preserve these parent nodes, set this flag to
   * true. Off by default.
   */
  svtkSetMacro(MaintainStructure, svtkTypeBool);
  svtkGetMacro(MaintainStructure, svtkTypeBool);
  svtkBooleanMacro(MaintainStructure, svtkTypeBool);
  //@}

protected:
  svtkExtractBlock();
  ~svtkExtractBlock() override;

  /**
   * Internal key, used to avoid pruning of a branch.
   */
  static svtkInformationIntegerKey* DONT_PRUNE();

  /// Implementation of the algorithm.
  int RequestData(svtkInformation*, svtkInformationVector**, svtkInformationVector*) override;

  /// Extract subtree
  void CopySubTree(
    svtkDataObjectTreeIterator* loc, svtkMultiBlockDataSet* output, svtkMultiBlockDataSet* input);
  bool Prune(svtkMultiBlockDataSet* mblock);
  bool Prune(svtkMultiPieceDataSet* mblock);
  bool Prune(svtkDataObject* mblock);

  svtkTypeBool PruneOutput;
  svtkTypeBool MaintainStructure;

private:
  svtkExtractBlock(const svtkExtractBlock&) = delete;
  void operator=(const svtkExtractBlock&) = delete;

  class svtkSet;
  svtkSet* Indices;
  svtkSet* ActiveIndices;
};

#endif
