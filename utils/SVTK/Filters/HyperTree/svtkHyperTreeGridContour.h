/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkHyperTreeGridContour.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   svtkHyperTreeGridContour
 * @brief   Extract cells from a hyper tree grid
 * where selected scalar value is within given range.
 *
 *
 * This filter extracts cells from a hyper tree grid that satisfy the
 * following contour: a cell is considered to be within range if its
 * value for the active scalar is within a specified range (inclusive).
 * The output remains a hyper tree grid.
 *
 * @sa
 * svtkHyperTreeGrid svtkHyperTreeGridAlgorithm svtkContourFilter
 *
 * @par Thanks:
 * This class was written by Guenole Harel and Jacques-Bernard Lekien 2014
 * This class was revised by Philippe Pebay, 2016
 * This class was modified by Jacques-Bernard Lekien, 2018
 * This work was supported by Commissariat a l'Energie Atomique
 * CEA, DAM, DIF, F-91297 Arpajon, France.
 */

#ifndef svtkHyperTreeGridContour_h
#define svtkHyperTreeGridContour_h

#include "svtkContourValues.h"          // Needed for inline methods
#include "svtkFiltersHyperTreeModule.h" // For export macro
#include "svtkHyperTreeGridAlgorithm.h"

#include <vector> // For STL

class svtkBitArray;
class svtkContourHelper;
class svtkDataArray;
class svtkHyperTreeGrid;
class svtkIdList;
class svtkIncrementalPointLocator;
class svtkLine;
class svtkPixel;
class svtkPointData;
class svtkUnsignedCharArray;
class svtkVoxel;
class svtkHyperTreeGridNonOrientedCursor;
class svtkHyperTreeGridNonOrientedMooreSuperCursor;

class SVTKFILTERSHYPERTREE_EXPORT svtkHyperTreeGridContour : public svtkHyperTreeGridAlgorithm
{
public:
  static svtkHyperTreeGridContour* New();
  svtkTypeMacro(svtkHyperTreeGridContour, svtkHyperTreeGridAlgorithm);
  void PrintSelf(ostream&, svtkIndent) override;

  //@{
  /**
   * Set / get a spatial locator for merging points. By default,
   * an instance of svtkMergePoints is used.
   */
  void SetLocator(svtkIncrementalPointLocator*);
  svtkGetObjectMacro(Locator, svtkIncrementalPointLocator);
  //@}

  /**
   * Create default locator. Used to create one when none is
   * specified. The locator is used to merge coincident points.
   */
  void CreateDefaultLocator();

  /**
   * Modified GetMTime Because we delegate to svtkContourValues.
   */
  svtkMTimeType GetMTime() override;

  //@{
  /**
   * Methods (inlined) to set / get contour values.
   */
  void SetValue(int, double);
  double GetValue(int);
  double* GetValues();
  void GetValues(double*);
  void SetNumberOfContours(int);
  svtkIdType GetNumberOfContours();
  void GenerateValues(int, double[2]);
  void GenerateValues(int, double, double);
  //@}

protected:
  svtkHyperTreeGridContour();
  ~svtkHyperTreeGridContour() override;

  /**
   * For this algorithm the output is a svtkPolyData instance
   */
  int FillOutputPortInformation(int, svtkInformation*) override;

  /**
   * Main routine to generate isocontours of hyper tree grid.
   */
  int ProcessTrees(svtkHyperTreeGrid*, svtkDataObject*) override;

  /**
   * Recursively decide whether a cell is intersected by a contour
   */
  bool RecursivelyPreProcessTree(svtkHyperTreeGridNonOrientedCursor*);

  /**
   * Recursively descend into tree down to leaves
   */
  void RecursivelyProcessTree(svtkHyperTreeGridNonOrientedMooreSuperCursor*);

  /**
   * Storage for contour values.
   */
  svtkContourValues* ContourValues;

  /**
   * Storage for pre-selected cells to be processed
   */
  svtkBitArray* SelectedCells;

  /**
   * Sign of isovalue if cell not treated
   */
  svtkBitArray** CellSigns;

  /**
   * Spatial locator to merge points.
   */
  svtkIncrementalPointLocator* Locator;

  //@{
  /**
   * Pointers needed to perform isocontouring
   */
  svtkContourHelper* Helper;
  svtkDataArray* CellScalars;
  svtkLine* Line;
  svtkPixel* Pixel;
  svtkVoxel* Voxel;
  svtkIdList* Leaves;
  //@}

  /**
   * Storage for signs relative to current contour value
   */
  std::vector<bool> Signs;

  /**
   * Keep track of current index in output polydata
   */
  svtkIdType CurrentId;

  /**
   * Keep track of selected input scalars
   */
  svtkDataArray* InScalars;

  svtkBitArray* InMask;
  svtkUnsignedCharArray* InGhostArray;

private:
  svtkHyperTreeGridContour(const svtkHyperTreeGridContour&) = delete;
  void operator=(const svtkHyperTreeGridContour&) = delete;
};

/**
 * Set a particular contour value at contour number i. The index i ranges
 * between 0<=i<NumberOfContours.
 */
inline void svtkHyperTreeGridContour::SetValue(int i, double value)
{
  this->ContourValues->SetValue(i, value);
}

/**
 * Get the ith contour value.
 */
inline double svtkHyperTreeGridContour::GetValue(int i)
{
  return this->ContourValues->GetValue(i);
}

/**
 * Get a pointer to an array of contour values. There will be
 * GetNumberOfContours() values in the list.
 */
inline double* svtkHyperTreeGridContour::GetValues()
{
  return this->ContourValues->GetValues();
}

/**
 * Fill a supplied list with contour values. There will be
 * GetNumberOfContours() values in the list. Make sure you allocate
 * enough memory to hold the list.
 */
inline void svtkHyperTreeGridContour::GetValues(double* contourValues)
{
  this->ContourValues->GetValues(contourValues);
}

/**
 * Set the number of contours to place into the list. You only really
 * need to use this method to reduce list size. The method SetValue()
 * will automatically increase list size as needed.
 */
inline void svtkHyperTreeGridContour::SetNumberOfContours(int number)
{
  this->ContourValues->SetNumberOfContours(number);
}

/**
 * Get the number of contours in the list of contour values.
 */
inline svtkIdType svtkHyperTreeGridContour::GetNumberOfContours()
{
  return this->ContourValues->GetNumberOfContours();
}

/**
 * Generate numContours equally spaced contour values between specified
 * range. Contour values will include min/max range values.
 */
inline void svtkHyperTreeGridContour::GenerateValues(int numContours, double range[2])
{
  this->ContourValues->GenerateValues(numContours, range);
}

/**
 * Generate numContours equally spaced contour values between specified
 * range. Contour values will include min/max range values.
 */
inline void svtkHyperTreeGridContour::GenerateValues(
  int numContours, double rangeStart, double rangeEnd)
{
  this->ContourValues->GenerateValues(numContours, rangeStart, rangeEnd);
}

#endif // svtkHyperTreeGridContour_h
