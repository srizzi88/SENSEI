/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkBinCellDataFilter.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   svtkBinCellDataFilter
 * @brief   bin source cell data into input cells.
 *
 * svtkBinCellDataFilter takes a source mesh containing scalar cell data, an
 * input mesh and a set of bin values and bins the source mesh's scalar cell
 * data into the cells of the input mesh. The resulting output mesh is identical
 * to the input mesh, with an additional cell data field, with tuple size equal
 * to the number of bins + 1, that represents a histogram of the cell data
 * values for all of the source cells whose centroid lie within the input cell.
 *
 * This filter is useful for analyzing the efficacy of an input mesh's ability
 * to represent the cell data of the source mesh.
 */

#ifndef svtkBinCellDataFilter_h
#define svtkBinCellDataFilter_h

#include "svtkDataSetAlgorithm.h"
#include "svtkDataSetAttributes.h" // needed for svtkDataSetAttributes::FieldList
#include "svtkFiltersCoreModule.h" // For export macro

#include "svtkContourValues.h" // Needed for inline methods

class svtkAbstractCellLocator;

class SVTKFILTERSCORE_EXPORT svtkBinCellDataFilter : public svtkDataSetAlgorithm
{
public:
  typedef svtkContourValues svtkBinValues;

  /**
   * Construct object with initial range (SVTK_DOUBLE_MIN, SVTK_DOUBLE_MAX) and
   * a single bin.
   */
  static svtkBinCellDataFilter* New();

  //@{
  /**
   * Standard methods for type and printing.
   */
  svtkTypeMacro(svtkBinCellDataFilter, svtkDataSetAlgorithm);
  void PrintSelf(ostream& os, svtkIndent indent) override;
  //@}

  //@{
  /**
   * Methods to set / get bin values.
   */
  void SetValue(int i, double value);
  double GetValue(int i);
  double* GetValues();
  void GetValues(double* binValues);
  void SetNumberOfBins(int numBins);
  svtkIdType GetNumberOfBins();
  void GenerateValues(int numBins, double range[2]);
  void GenerateValues(int numBins, double rangeStart, double rangeEnd);
  //@}

  //@{
  /**
   * Specify the data set whose cells will be counted.
   * The Input gives the geometry (the points and cells) for the output,
   * while the Source is used to determine how many source cells lie within
   * each input cell.
   */
  void SetSourceData(svtkDataObject* source);
  svtkDataObject* GetSource();
  //@}

  /**
   * Specify the data set whose cells will be counted.
   * The Input gives the geometry (the points and cells) for the output,
   * while the Source is used to determine how many source cells lie within
   * each input cell.
   */
  void SetSourceConnection(svtkAlgorithmOutput* algOutput);

  //@{
  /**
   * This flag is used only when a piece is requested to update.  By default
   * the flag is off.  Because no spatial correspondence between input pieces
   * and source pieces is known, all of the source has to be requested no
   * matter what piece of the output is requested.  When there is a spatial
   * correspondence, the user/application can set this flag.  This hint allows
   * the breakup of the probe operation to be much more efficient.  When piece
   * m of n is requested for update by the user, then only n of m needs to
   * be requested of the source.
   */
  svtkSetMacro(SpatialMatch, svtkTypeBool);
  svtkGetMacro(SpatialMatch, svtkTypeBool);
  svtkBooleanMacro(SpatialMatch, svtkTypeBool);
  //@}

  //@{
  /**
   * Set whether to store the number of nonzero bins for each cell.
   * On by default.
   */
  svtkSetMacro(StoreNumberOfNonzeroBins, bool);
  svtkBooleanMacro(StoreNumberOfNonzeroBins, bool);
  svtkGetMacro(StoreNumberOfNonzeroBins, bool);
  //@}

  //@{
  /**
   * Returns the name of the id array added to the output that holds the number
   * of nonzero bins per cell.
   * Set to "NumberOfNonzeroBins" by default.
   */
  svtkSetStringMacro(NumberOfNonzeroBinsArrayName);
  svtkGetStringMacro(NumberOfNonzeroBinsArrayName);
  //@}

  //@{
  /**
   * Set the tolerance used to compute whether a cell centroid in the
   * source is in a cell of the input.  This value is only used
   * if ComputeTolerance is off.
   */
  svtkSetMacro(Tolerance, double);
  svtkGetMacro(Tolerance, double);
  //@}

  //@{
  /**
   * Set whether to use the Tolerance field or precompute the tolerance.
   * When on, the tolerance will be computed and the field value is ignored.
   * Off by default.
   */
  svtkSetMacro(ComputeTolerance, bool);
  svtkBooleanMacro(ComputeTolerance, bool);
  svtkGetMacro(ComputeTolerance, bool);
  //@}

  //@{
  /**
   * Set/get which component of the scalar array to bin; defaults to 0.
   */
  svtkSetMacro(ArrayComponent, int);
  svtkGetMacro(ArrayComponent, int);
  //@}

  enum CellOverlapCriterion
  {
    CELL_CENTROID = 0,
    CELL_POINTS = 1,
  };

  //@{
  /**
   * Set whether cell overlap is determined by source cell centroid or by source
   * cell points.
   * Centroid by default.
   */
  svtkSetClampMacro(CellOverlapMethod, int, CELL_CENTROID, CELL_POINTS);
  svtkGetMacro(CellOverlapMethod, int);
  //@}

  //@{
  /**
   * Set/Get a spatial locator for speeding the search process. By
   * default an instance of svtkStaticCellLocator is used.
   */
  virtual void SetCellLocator(svtkAbstractCellLocator* cellLocator);
  svtkGetObjectMacro(CellLocator, svtkAbstractCellLocator);
  //@}

protected:
  svtkBinCellDataFilter();
  ~svtkBinCellDataFilter() override;

  svtkTypeBool SpatialMatch;

  bool StoreNumberOfNonzeroBins;
  double Tolerance;
  bool ComputeTolerance;
  int ArrayComponent;
  int CellOverlapMethod;

  svtkBinValues* BinValues;
  svtkAbstractCellLocator* CellLocator;
  virtual void CreateDefaultLocator();

  int RequestData(svtkInformation*, svtkInformationVector**, svtkInformationVector*) override;
  int RequestInformation(svtkInformation*, svtkInformationVector**, svtkInformationVector*) override;
  int RequestUpdateExtent(svtkInformation*, svtkInformationVector**, svtkInformationVector*) override;

  char* NumberOfNonzeroBinsArrayName;

private:
  svtkBinCellDataFilter(const svtkBinCellDataFilter&) = delete;
  void operator=(const svtkBinCellDataFilter&) = delete;
};

/**
 * Set a particular bin value at bin number i. The index i ranges
 * between 0<=i<NumberOfBins.
 */
inline void svtkBinCellDataFilter::SetValue(int i, double value)
{
  this->BinValues->SetValue(i, value);
}

/**
 * Get the ith bin value.
 */
inline double svtkBinCellDataFilter::GetValue(int i)
{
  return this->BinValues->GetValue(i);
}

/**
 * Get a pointer to an array of bin values. There will be
 * GetNumberOfBins() values in the list.
 */
inline double* svtkBinCellDataFilter::GetValues()
{
  return this->BinValues->GetValues();
}

/**
 * Fill a supplied list with bin values. There will be
 * GetNumberOfBins() values in the list. Make sure you allocate
 * enough memory to hold the list.
 */
inline void svtkBinCellDataFilter::GetValues(double* binValues)
{
  this->BinValues->GetValues(binValues);
}

/**
 * Set the number of bins to place into the list. You only really
 * need to use this method to reduce list size. The method SetValue()
 * will automatically increase list size as needed.
 */
inline void svtkBinCellDataFilter::SetNumberOfBins(int number)
{
  this->BinValues->SetNumberOfContours(number);
}

/**
 * Get the number of bins in the list of bin values, not counting the overflow
 * bin.
 */
inline svtkIdType svtkBinCellDataFilter::GetNumberOfBins()
{
  return this->BinValues->GetNumberOfContours();
}

/**
 * Generate numBins equally spaced bin values between specified
 * range. Bin values will include min/max range values.
 */
inline void svtkBinCellDataFilter::GenerateValues(int numBins, double range[2])
{
  this->BinValues->GenerateValues(numBins, range);
}

/**
 * Generate numBins equally spaced bin values between specified
 * range. Bin values will include min/max range values.
 */
inline void svtkBinCellDataFilter::GenerateValues(int numBins, double rangeStart, double rangeEnd)
{
  this->BinValues->GenerateValues(numBins, rangeStart, rangeEnd);
}

#endif
