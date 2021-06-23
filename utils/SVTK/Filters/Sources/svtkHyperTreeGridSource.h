/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkHyperTreeGridSource.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   svtkHyperTreeGridSource
 * @brief   Create a synthetic grid of hypertrees.
 *
 *
 * This class uses input parameters, most notably a string descriptor,
 * to generate a svtkHyperTreeGrid instance representing the corresponding
 * tree-based AMR grid. This descriptor uses the following conventions,
 * e.g., to describe a 1-D ternary subdivision with 2 root cells
 * L0    L1        L2
 * RR  | .R. ... | ...
 * For this tree:
 *  HTG:       .
 *           /   \
 *  L0:     .     .
 *         /|\   /|\
 *  L1:   c . c c c c
 *         /|\
 *  L2:   c c c
 * The top level of the tree is not considered a grid level
 * NB: For ease of legibility, white spaces are allowed and ignored.
 *
 * @par Thanks:
 * This class was written by Philippe Pebay, Joachim Pouderoux, and Charles Law, Kitware 2013
 * This class was modified by Guenole Harel and Jacques-Bernard Lekien 2014
 * This class was modified by Philippe Pebay, 2016
 * This work was supported by Commissariat a l'Energie Atomique (CEA/DIF)
 * CEA, DAM, DIF, F-91297 Arpajon, France.
 */

#ifndef svtkHyperTreeGridSource_h
#define svtkHyperTreeGridSource_h

#include "svtkFiltersSourcesModule.h" // For export macro
#include "svtkHyperTreeGridAlgorithm.h"

#include <map>    // STL Header
#include <string> // STL Header
#include <vector> // STL Header

class svtkBitArray;
class svtkDataArray;
class svtkHyperTreeGridNonOrientedCursor;
class svtkIdTypeArray;
class svtkImplicitFunction;
class svtkHyperTreeGrid;
class svtkQuadric;

class SVTKFILTERSSOURCES_EXPORT svtkHyperTreeGridSource : public svtkHyperTreeGridAlgorithm
{
public:
  svtkTypeMacro(svtkHyperTreeGridSource, svtkHyperTreeGridAlgorithm);
  void PrintSelf(ostream& os, svtkIndent indent) override;

  static svtkHyperTreeGridSource* New();

  // @deprecated Replaced by GetMaxDepth() as of SVTK 9
  SVTK_LEGACY(unsigned int GetMaximumLevel());

  // @deprecated Replaced by SetMaxDepth() as of SVTK 9
  SVTK_LEGACY(void SetMaximumLevel(unsigned int levels));

  /**
   * Return the maximum number of levels of the hypertree.
   * \post positive_result: result>=1
   */
  unsigned int GetMaxDepth();

  /**
   * Set the maximum number of levels of the hypertrees.
   * \pre positive_levels: levels>=1
   * \post is_set: this->GetLevels()==levels
   * \post min_is_valid: this->GetMinLevels()<this->GetLevels()
   */
  void SetMaxDepth(unsigned int levels);

  //@{
  /**
   * Set/Get the origin of the grid
   */
  svtkSetVector3Macro(Origin, double);
  svtkGetVector3Macro(Origin, double);
  //@}

  //@{
  /**
   * Set/Get the scale to be applied to root cells in each dimension of the grid
   */
  svtkSetVector3Macro(GridScale, double);
  svtkGetVector3Macro(GridScale, double);
  void SetGridScale(double scale) { this->SetGridScale(scale, scale, scale); }
  //@}

  //@{
  /**
   * Set/Get the number of root cells + 1 in each dimension of the grid
   */
  void SetDimensions(const unsigned int* dims);
  void SetDimensions(unsigned int, unsigned int, unsigned int);
  svtkGetVector3Macro(Dimensions, unsigned int);
  //@}

  //@{
  /**
   * Specify whether indexing mode of grid root cells must be transposed to
   * x-axis first, z-axis last, instead of the default z-axis first, k-axis last
   */
  svtkSetMacro(TransposedRootIndexing, bool);
  svtkGetMacro(TransposedRootIndexing, bool);
  void SetIndexingModeToKJI();
  void SetIndexingModeToIJK();
  //@}

  //@{
  /**
   * Set/Get the orientation of the grid (in 1D and 2D)
   */
  svtkGetMacro(Orientation, unsigned int);
  //@}

  //@{
  /**
   * Set/Get the subdivision factor in the grid refinement scheme
   */
  svtkSetClampMacro(BranchFactor, unsigned int, 2, 3);
  svtkGetMacro(BranchFactor, unsigned int);
  //@}

  //@{
  /**
   * Set/get whether the descriptor string should be used.
   * NB: Otherwise a quadric definition is expected.
   * Default: true
   */
  svtkSetMacro(UseDescriptor, bool);
  svtkGetMacro(UseDescriptor, bool);
  svtkBooleanMacro(UseDescriptor, bool);
  //@}

  //@{
  /**
   * Set/get whether the material mask should be used.
   * NB: This is only used when UseDescriptor is ON
   * Default: false
   */
  svtkSetMacro(UseMask, bool);
  svtkGetMacro(UseMask, bool);
  svtkBooleanMacro(UseMask, bool);
  //@}

  //@{
  /**
   * Set/get whether cell-centered interface fields
   * should be generated.
   * Default: false
   */
  svtkSetMacro(GenerateInterfaceFields, bool);
  svtkGetMacro(GenerateInterfaceFields, bool);
  svtkBooleanMacro(GenerateInterfaceFields, bool);
  //@}

  //@{
  /**
   * Set/Get the string used to describe the grid.
   */
  svtkSetStringMacro(Descriptor);
  svtkGetStringMacro(Descriptor);
  //@}

  //@{
  /**
   * Set/Get the string used to as a material mask.
   */
  svtkSetStringMacro(Mask);
  svtkGetStringMacro(Mask);
  //@}

  //@{
  /**
   * Set/Get the bitarray used to describe the grid.
   */
  virtual void SetDescriptorBits(svtkBitArray*);
  svtkGetObjectMacro(DescriptorBits, svtkBitArray);
  //@}

  /**
   * Set the index array used to as a material mask.
   */
  virtual void SetLevelZeroMaterialIndex(svtkIdTypeArray*);

  //@{
  /**
   * Set/Get the bitarray used as a material mask.
   */
  virtual void SetMaskBits(svtkBitArray*);
  svtkGetObjectMacro(MaskBits, svtkBitArray);
  //@}

  //@{
  /**
   * Set/Get the quadric function.
   */
  virtual void SetQuadric(svtkQuadric*);
  svtkGetObjectMacro(Quadric, svtkQuadric);
  //@}

  //@{
  /**
   * Helpers to set/get the 10 coefficients of the quadric function
   */
  void SetQuadricCoefficients(double[10]);
  void GetQuadricCoefficients(double[10]);
  double* GetQuadricCoefficients();
  //@}

  /**
   * Override GetMTime because we delegate to a svtkQuadric.
   */
  svtkMTimeType GetMTime() override;

  //@{
  /**
   * Helpers to convert string descriptors & mask to bit arrays
   */
  svtkBitArray* ConvertDescriptorStringToBitArray(const std::string&);
  svtkBitArray* ConvertMaskStringToBitArray(const std::string&);
  //@}

protected:
  svtkHyperTreeGridSource();
  ~svtkHyperTreeGridSource() override;

  int RequestInformation(svtkInformation*, svtkInformationVector**, svtkInformationVector*) override;

  int RequestData(svtkInformation*, svtkInformationVector**, svtkInformationVector*) override;

  int FillOutputPortInformation(int, svtkInformation*) override;

  /**
   * Main routine to process individual trees in the grid
   */
  int ProcessTrees(svtkHyperTreeGrid*, svtkDataObject*) override;

  /**
   * Initialize grid from descriptor string when it is to be used
   */
  int InitializeFromStringDescriptor();

  /**
   * Initialize grid from bit array descriptors when it is to be used
   */
  int InitializeFromBitsDescriptor();

  /**
   * Initialize tree grid from descriptor and call subdivide if needed
   */
  void InitTreeFromDescriptor(
    svtkHyperTreeGrid* output, svtkHyperTreeGridNonOrientedCursor* cursor, int treeIdx, int idx[3]);

  /**
   * Subdivide grid from descriptor string when it is to be used
   */
  void SubdivideFromStringDescriptor(svtkHyperTreeGrid* output,
    svtkHyperTreeGridNonOrientedCursor* cursor, unsigned int level, int treeIdx, int childIdx,
    int idx[3], int parentPos);

  /**
   * Subdivide grid from descriptor string when it is to be used
   */
  void SubdivideFromBitsDescriptor(svtkHyperTreeGrid* output,
    svtkHyperTreeGridNonOrientedCursor* cursor, unsigned int level, int treeIdx, int childIdx,
    int idx[3], int parentPos);

  /**
   * Subdivide grid from quadric when descriptor is not used
   */
  void SubdivideFromQuadric(svtkHyperTreeGrid* output, svtkHyperTreeGridNonOrientedCursor* cursor,
    unsigned int level, int treeIdx, const int idx[3], double origin[3], double size[3]);

  /**
   * Evaluate quadric at given point coordinates
   */
  double EvaluateQuadric(double[3]);

  double Origin[3];
  double GridScale[3];
  unsigned int Dimension;

protected:
  unsigned int Dimensions[3];
  bool TransposedRootIndexing;
  unsigned int MaxDepth;

  unsigned int Orientation;
  unsigned int BranchFactor;
  unsigned int BlockSize;
  bool UseDescriptor;
  bool UseMask;
  bool GenerateInterfaceFields;

  svtkDataArray* XCoordinates;
  svtkDataArray* YCoordinates;
  svtkDataArray* ZCoordinates;

  char* Descriptor;
  char* Mask;
  std::vector<std::string> LevelDescriptors;
  std::vector<std::string> LevelMasks;

  svtkBitArray* DescriptorBits;
  svtkBitArray* MaskBits;
  std::vector<svtkIdType> LevelBitsIndex;
  std::vector<svtkIdType> LevelBitsIndexCnt;

  svtkIdTypeArray* LevelZeroMaterialIndex;
  std::map<svtkIdType, svtkIdType> LevelZeroMaterialMap;

  std::vector<int> LevelCounters;

  svtkQuadric* Quadric;

  svtkHyperTreeGrid* OutputHTG;

private:
  svtkHyperTreeGridSource(const svtkHyperTreeGridSource&) = delete;
  void operator=(const svtkHyperTreeGridSource&) = delete;
};

#endif
