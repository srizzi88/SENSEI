/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkHyperTreeGridDepthLimiter.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   svtkHyperTreeGridDepthLimiter
 * @brief   Hyper tree grid level extraction
 *
 *
 * Extract all levels down to a specified depth from a hyper tree grid.
 * If the required depth is greater or equal to the maximum level of the
 * input grid, then the output is identical.
 * Note that when a material mask is present, the geometry extent of the
 * output grid is guaranteed to contain that of the input tree, but the
 * former might be strictly larger than the latter. This is not a bug
 * but an expected behavior of which the user should be aware.
 *
 * @sa
 * svtkHyperTreeGrid svtkHyperTreeGridAlgorithm
 *
 * @par Thanks:
 * This class was written by Guenole Harel and Jacques-Bernard Lekien 2014
 * This class was modified by Philippe Pebay, 2016
 * This class was modified by Jacques-Bernard Lekien, 2018
 * This class was optimized by Jacques-Bernard Lekien, 2019,
 * by DepthLimiter directly manadged by HyperTreeGrid and (super)cursors.
 * This work was supported by Commissariat a l'Energie Atomique
 * CEA, DAM, DIF, F-91297 Arpajon, France.
 */

#ifndef svtkHyperTreeGridDepthLimiter_h
#define svtkHyperTreeGridDepthLimiter_h

#include "svtkFiltersHyperTreeModule.h" // For export macro
#include "svtkHyperTreeGridAlgorithm.h"

class svtkBitArray;
class svtkHyperTreeGrid;
class svtkHyperTreeGridNonOrientedCursor;

class SVTKFILTERSHYPERTREE_EXPORT svtkHyperTreeGridDepthLimiter : public svtkHyperTreeGridAlgorithm
{
public:
  static svtkHyperTreeGridDepthLimiter* New();
  svtkTypeMacro(svtkHyperTreeGridDepthLimiter, svtkHyperTreeGridAlgorithm);
  void PrintSelf(ostream&, svtkIndent) override;

  //@{
  /**
   * Set/Get True, create a new mask ; false, create a new HTG.
   */
  svtkSetMacro(JustCreateNewMask, bool);
  svtkGetMacro(JustCreateNewMask, bool);
  //@}

  //@{
  /**
   * Set/Get maximum depth to which output grid should be limited
   */
  svtkSetMacro(Depth, unsigned int);
  svtkGetMacro(Depth, unsigned int);
  //@}

protected:
  svtkHyperTreeGridDepthLimiter();
  ~svtkHyperTreeGridDepthLimiter() override;

  /**
   * For this algorithm the output is a svtkHyperTreeGrid instance
   */
  int FillOutputPortInformation(int, svtkInformation*) override;

  /**
   * Main routine to extract hyper tree grid levels
   */
  int ProcessTrees(svtkHyperTreeGrid*, svtkDataObject*) override;

  /**
   * Recursively descend into tree down to leaves
   */
  void RecursivelyProcessTree(
    svtkHyperTreeGridNonOrientedCursor*, svtkHyperTreeGridNonOrientedCursor*);

  /**
   * Maximum depth of hyper tree grid to be extracted
   */
  unsigned int Depth;

  /**
   * Input mask
   */
  svtkBitArray* InMask;

  /**
   * Output mask constructed by this filter
   */
  svtkBitArray* OutMask;

  /**
   * Keep track of current index in output hyper tree grid
   */
  svtkIdType CurrentId;

  /**
   * With or without copy
   */
  bool JustCreateNewMask;

private:
  svtkHyperTreeGridDepthLimiter(const svtkHyperTreeGridDepthLimiter&) = delete;
  void operator=(const svtkHyperTreeGridDepthLimiter&) = delete;
};

#endif // svtkHyperTreeGridDepthLimiter_h
