/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkRandomHyperTreeGridSource.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class svtkRandomHyperTreeGridSource
 * @brief Builds a randomized but reproducible svtkHyperTreeGrid.
 */

#ifndef svtkRandomHyperTreeGridSource_h
#define svtkRandomHyperTreeGridSource_h

#include "svtkFiltersSourcesModule.h" // For export macro
#include "svtkHyperTreeGridAlgorithm.h"

#include <svtkNew.h> // For svtkNew

class svtkDoubleArray;
class svtkExtentTranslator;
class svtkHyperTreeGridNonOrientedCursor;
class svtkMinimalStandardRandomSequence;

class SVTKFILTERSSOURCES_EXPORT svtkRandomHyperTreeGridSource : public svtkHyperTreeGridAlgorithm
{
public:
  static svtkRandomHyperTreeGridSource* New();
  svtkTypeMacro(svtkRandomHyperTreeGridSource, svtkHyperTreeGridAlgorithm);
  void PrintSelf(ostream& os, svtkIndent indent) override;

  /**
   * The Dimensions of the output svtkHyperTreeGrid.
   * Default is 5x5x2.
   * @{
   */
  svtkGetVector3Macro(Dimensions, unsigned int);
  svtkSetVector3Macro(Dimensions, unsigned int);
  /**@}*/

  /**
   * The bounds of the output svtkHyperTreeGrid.
   * The default is {-10, 10, -10, 10, -10, 10}.
   */
  svtkGetVector6Macro(OutputBounds, double);
  svtkSetVector6Macro(OutputBounds, double);

  /**
   * A seed for the random number generator used to construct the output
   * svtkHyperTreeGrid.
   * The default is 0.
   * @{
   */
  svtkGetMacro(Seed, svtkTypeUInt32);
  svtkSetMacro(Seed, svtkTypeUInt32);
  /**@}*/

  /**
   * The maximum number of levels to allow in the output svtkHyperTreeGrid.
   * The default is 5.
   * @{
   */
  svtkGetMacro(MaxDepth, svtkIdType);
  svtkSetClampMacro(MaxDepth, svtkIdType, 1, SVTK_ID_MAX);
  /**@}*/

  /**
   * The target fraction of nodes that will be split during generation.
   * Valid range is [0., 1.]. The default is 0.5.
   * @{
   */
  svtkGetMacro(SplitFraction, double);
  svtkSetClampMacro(SplitFraction, double, 0., 1.);
  /**@}*/

protected:
  svtkRandomHyperTreeGridSource();
  ~svtkRandomHyperTreeGridSource() override;

  int RequestInformation(
    svtkInformation* req, svtkInformationVector** inInfo, svtkInformationVector* outInfo) override;

  int RequestData(
    svtkInformation* req, svtkInformationVector** inInfo, svtkInformationVector* outInfo) override;

  // We just do the work in RequestData.
  int ProcessTrees(svtkHyperTreeGrid*, svtkDataObject*) final { return 1; }

  int FillOutputPortInformation(int port, svtkInformation* info) override;

  void SubdivideLeaves(svtkHyperTreeGridNonOrientedCursor* cursor, svtkIdType treeId);

  bool ShouldRefine(svtkIdType level);

  unsigned int Dimensions[3];
  double OutputBounds[6];
  svtkTypeUInt32 Seed;
  svtkIdType MaxDepth;
  double SplitFraction;

private:
  svtkRandomHyperTreeGridSource(const svtkRandomHyperTreeGridSource&) = delete;
  void operator=(const svtkRandomHyperTreeGridSource&) = delete;

  svtkNew<svtkMinimalStandardRandomSequence> RNG;
  svtkNew<svtkExtentTranslator> ExtentTranslator;
  svtkDoubleArray* Levels;
};

#endif // svtkRandomHyperTreeGridSource_h
