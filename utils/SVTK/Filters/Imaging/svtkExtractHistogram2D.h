/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkExtractHistogram2D.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/*-------------------------------------------------------------------------
  Copyright 2011 Sandia Corporation.
  Under the terms of Contract DE-AC04-94AL85000 with Sandia Corporation,
  the U.S. Government retains certain rights in this software.
-------------------------------------------------------------------------*/
/**
 * @class   svtkExtractHistogram2D
 * @brief   compute a 2D histogram between two columns
 *  of an input svtkTable.
 *
 *
 *  This class computes a 2D histogram between two columns of an input
 *  svtkTable. Just as with a 1D histogram, a 2D histogram breaks
 *  up the input domain into bins, and each pair of values (row in
 *  the table) fits into a single bin and increments a row counter
 *  for that bin.
 *
 *  To use this class, set the input with a table and call AddColumnPair(nameX,nameY),
 *  where nameX and nameY are the names of the two columns to be used.
 *
 *  In addition to the number of bins (in X and Y), the domain of
 *  the histogram can be customized by toggling the UseCustomHistogramExtents
 *  flag and setting the CustomHistogramExtents variable to the
 *  desired value.
 *
 * @sa
 *  svtkPExtractHistogram2D
 *
 * @par Thanks:
 *  Developed by David Feng and Philippe Pebay at Sandia National Laboratories
 *------------------------------------------------------------------------------
 */

#ifndef svtkExtractHistogram2D_h
#define svtkExtractHistogram2D_h

#include "svtkFiltersImagingModule.h" // For export macro
#include "svtkStatisticsAlgorithm.h"

class svtkImageData;
class svtkIdTypeArray;
class svtkMultiBlockDataSet;

class SVTKFILTERSIMAGING_EXPORT svtkExtractHistogram2D : public svtkStatisticsAlgorithm
{
public:
  static svtkExtractHistogram2D* New();
  svtkTypeMacro(svtkExtractHistogram2D, svtkStatisticsAlgorithm);
  void PrintSelf(ostream& os, svtkIndent indent) override;

  enum OutputIndices
  {
    HISTOGRAM_IMAGE = 3
  };

  //@{
  /**
   * Set/get the number of bins to be used per dimension (x,y)
   */
  svtkSetVector2Macro(NumberOfBins, int);
  svtkGetVector2Macro(NumberOfBins, int);
  //@}

  //@{
  /**
   * Set/get the components of the arrays in the two input columns
   * to be used during histogram computation.  Defaults to component 0.
   */
  svtkSetVector2Macro(ComponentsToProcess, int);
  svtkGetVector2Macro(ComponentsToProcess, int);
  //@}

  //@{
  /**
   * Set/get a custom domain for histogram computation.  UseCustomHistogramExtents
   * must be called for these to actually be used.
   */
  svtkSetVector4Macro(CustomHistogramExtents, double);
  svtkGetVector4Macro(CustomHistogramExtents, double);
  //@}

  //@{
  /**
   * Use the extents in CustomHistogramExtents when computing the
   * histogram, rather than the simple range of the input columns.
   */
  svtkSetMacro(UseCustomHistogramExtents, svtkTypeBool);
  svtkGetMacro(UseCustomHistogramExtents, svtkTypeBool);
  svtkBooleanMacro(UseCustomHistogramExtents, svtkTypeBool);
  //@}

  //@{
  /**
   * Control the scalar type of the output histogram.  If the input
   * is relatively small, you can save space by using a smaller
   * data type.  Defaults to unsigned integer.
   */
  svtkSetMacro(ScalarType, int);
  void SetScalarTypeToUnsignedInt() { this->SetScalarType(SVTK_UNSIGNED_INT); }
  void SetScalarTypeToUnsignedLong() { this->SetScalarType(SVTK_UNSIGNED_LONG); }
  void SetScalarTypeToUnsignedShort() { this->SetScalarType(SVTK_UNSIGNED_SHORT); }
  void SetScalarTypeToUnsignedChar() { this->SetScalarType(SVTK_UNSIGNED_CHAR); }
  void SetScalarTypeToFloat() { this->SetScalarType(SVTK_FLOAT); }
  void SetScalarTypeToDouble() { this->SetScalarType(SVTK_DOUBLE); }
  svtkGetMacro(ScalarType, int);
  //@}

  //@{
  /**
   * Access the count of the histogram bin containing the largest number
   * of input rows.
   */
  svtkGetMacro(MaximumBinCount, double);
  //@}

  /**
   * Compute the range of the bin located at position (binX,binY) in
   * the 2D histogram.
   */
  int GetBinRange(svtkIdType binX, svtkIdType binY, double range[4]);

  /**
   * Get the range of the of the bin located at 1D position index bin
   * in the 2D histogram array.
   */
  int GetBinRange(svtkIdType bin, double range[4]);

  /**
   * Get the width of all of the bins. Also stored in the spacing
   * ivar of the histogram image output.
   */
  void GetBinWidth(double bw[2]);

  /**
   * Gets the data object at the histogram image output port and
   * casts it to a svtkImageData.
   */
  svtkImageData* GetOutputHistogramImage();

  /**
   * Get the histogram extents currently in use, either computed
   * or set by the user.
   */
  double* GetHistogramExtents();

  svtkSetMacro(SwapColumns, svtkTypeBool);
  svtkGetMacro(SwapColumns, svtkTypeBool);
  svtkBooleanMacro(SwapColumns, svtkTypeBool);

  //@{
  /**
   * Get/Set an optional mask that can ignore rows of the table
   */
  virtual void SetRowMask(svtkDataArray*);
  svtkGetObjectMacro(RowMask, svtkDataArray);
  //@}

  /**
   * Given a collection of models, calculate aggregate model. Not used.
   */
  void Aggregate(svtkDataObjectCollection*, svtkMultiBlockDataSet*) override {}

protected:
  svtkExtractHistogram2D();
  ~svtkExtractHistogram2D() override;

  svtkTypeBool SwapColumns;
  int NumberOfBins[2];
  double HistogramExtents[4];
  double CustomHistogramExtents[4];
  svtkTypeBool UseCustomHistogramExtents;
  int ComponentsToProcess[2];
  double MaximumBinCount;
  int ScalarType;
  svtkDataArray* RowMask;

  virtual int ComputeBinExtents(svtkDataArray* col1, svtkDataArray* col2);

  /**
   * Execute the calculations required by the Learn option.
   * This is what actually does the histogram computation.
   */
  void Learn(svtkTable* inData, svtkTable* inParameters, svtkMultiBlockDataSet* inMeta) override;

  /**
   * Execute the calculations required by the Derive option. Not used.
   */
  void Derive(svtkMultiBlockDataSet*) override {}

  /**
   * Execute the calculations required by the Test option.
   */
  void Test(svtkTable*, svtkMultiBlockDataSet*, svtkTable*) override { return; }

  /**
   * Execute the calculations required by the Assess option.
   */
  void Assess(svtkTable*, svtkMultiBlockDataSet*, svtkTable*) override { return; }

  /**
   * Provide the appropriate assessment functor. Not used.
   */
  void SelectAssessFunctor(svtkTable* svtkNotUsed(outData), svtkDataObject* svtkNotUsed(inMeta),
    svtkStringArray* svtkNotUsed(rowNames), AssessFunctor*& svtkNotUsed(dfunc)) override
  {
  }

  int FillOutputPortInformation(int port, svtkInformation* info) override;

  /**
   * Makes sure that the image data output port has up-to-date spacing/origin/etc
   */
  int RequestInformation(svtkInformation* request, svtkInformationVector** inputVector,
    svtkInformationVector* outputVector) override;

  /**
   * Get points to the arrays that live in the two input columns
   */
  int GetInputArrays(svtkDataArray*& col1, svtkDataArray*& col2);

private:
  svtkExtractHistogram2D(const svtkExtractHistogram2D&) = delete;
  void operator=(const svtkExtractHistogram2D&) = delete;
};

#endif
