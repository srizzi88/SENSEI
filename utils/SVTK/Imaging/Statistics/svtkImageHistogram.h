/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkImageHistogram.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   svtkImageHistogram
 * @brief   Compute the histogram for an image.
 *
 * svtkImageHistogram generates a histogram from its input, and optionally
 * produces a 2D black-and-white image of the histogram as its output.
 * Unlike the class svtkImageAccumulate, a multi-component image does not
 * result in a multi-dimensional histogram.  Instead, the resulting
 * histogram will be the sum of the histograms of each of the individual
 * components, unless SetActiveComponent is used to choose a single
 * component.
 * @par Thanks:
 * Thanks to David Gobbi at the Seaman Family MR Centre and Dept. of Clinical
 * Neurosciences, Foothills Medical Centre, Calgary, for providing this class.
 */

#ifndef svtkImageHistogram_h
#define svtkImageHistogram_h

#include "svtkImagingStatisticsModule.h" // For export macro
#include "svtkThreadedImageAlgorithm.h"

class svtkImageStencilData;
class svtkIdTypeArray;
class svtkImageHistogramThreadData;
class svtkImageHistogramSMPThreadLocal;

class SVTKIMAGINGSTATISTICS_EXPORT svtkImageHistogram : public svtkThreadedImageAlgorithm
{
public:
  static svtkImageHistogram* New();
  svtkTypeMacro(svtkImageHistogram, svtkThreadedImageAlgorithm);

  void PrintSelf(ostream& os, svtkIndent indent) override;

  /**
   * Scale types for the histogram image.
   */
  enum
  {
    Linear = 0,
    Log = 1,
    Sqrt = 2
  };

  //@{
  /**
   * Set the component for which to generate a histogram.  The default
   * value is -1, which produces a histogram that is the sum of the
   * histograms of the individual components.
   */
  svtkSetMacro(ActiveComponent, int);
  svtkGetMacro(ActiveComponent, int);
  //@}

  //@{
  /**
   * If this is On, then the histogram binning will be done automatically.
   * For char and unsigned char data, there will be 256 bins with unit
   * spacing.  For data of type short and larger, there will be between
   * 256 and MaximumNumberOfBins, depending on the range of the data, and
   * the BinOrigin will be set to zero if no negative values are present,
   * or to the smallest negative value if negative values are present.
   * For float data, the MaximumNumberOfBins will always be used.
   * The BinOrigin and BinSpacing will be set so that they provide a mapping
   * from bin index to scalar value.
   */
  svtkSetMacro(AutomaticBinning, svtkTypeBool);
  svtkBooleanMacro(AutomaticBinning, svtkTypeBool);
  svtkGetMacro(AutomaticBinning, svtkTypeBool);
  //@}

  //@{
  /**
   * The maximum number of bins to use when AutomaticBinning is On.
   * When AutomaticBinning is On, the size of the output histogram
   * will be set to the full range of the input data values, unless
   * the full range is greater than this value.  By default, the max
   * value is 65536, which is large enough to capture the full range
   * of 16-bit integers.
   */
  svtkSetMacro(MaximumNumberOfBins, int);
  svtkGetMacro(MaximumNumberOfBins, int);
  //@}

  //@{
  /**
   * The number of bins in histogram (default 256).  This is automatically
   * computed unless AutomaticBinning is Off.
   */
  svtkSetMacro(NumberOfBins, int);
  svtkGetMacro(NumberOfBins, int);
  //@}

  //@{
  /**
   * The value for the center of the first bin (default 0).  This is
   * automatically computed unless AutomaticBinning is Off.
   */
  svtkSetMacro(BinOrigin, double);
  svtkGetMacro(BinOrigin, double);
  //@}

  //@{
  /**
   * The bin spacing (default 1).  This is automatically computed unless
   * AutomaticBinning is Off.
   */
  svtkSetMacro(BinSpacing, double);
  svtkGetMacro(BinSpacing, double);
  //@}

  //@{
  /**
   * Use a stencil to compute the histogram for just a part of the image.
   */
  void SetStencilData(svtkImageStencilData* stencil);
  svtkImageStencilData* GetStencil();
  //@}

  /**
   * Equivalent to SetInputConnection(1, algOutput).
   */
  void SetStencilConnection(svtkAlgorithmOutput* algOutput);

  //@{
  /**
   * If this is On, then a histogram image will be produced as the output.
   * Regardless of this setting, the histogram is always available as a
   * svtkIdTypeArray from the GetHistogram method.
   */
  svtkSetMacro(GenerateHistogramImage, svtkTypeBool);
  svtkBooleanMacro(GenerateHistogramImage, svtkTypeBool);
  svtkGetMacro(GenerateHistogramImage, svtkTypeBool);
  //@}

  //@{
  /**
   * Set the size of the histogram image that is produced as output.
   * The default is 256 by 256.
   */
  svtkSetVector2Macro(HistogramImageSize, int);
  svtkGetVector2Macro(HistogramImageSize, int);
  //@}

  //@{
  /**
   * Set the scale to use for the histogram image.  The default is
   * a linear scale, but sqrt and log provide better visualization.
   */
  svtkSetClampMacro(HistogramImageScale, int, svtkImageHistogram::Linear, svtkImageHistogram::Sqrt);
  void SetHistogramImageScaleToLinear() { this->SetHistogramImageScale(svtkImageHistogram::Linear); }
  void SetHistogramImageScaleToLog() { this->SetHistogramImageScale(svtkImageHistogram::Log); }
  void SetHistogramImageScaleToSqrt() { this->SetHistogramImageScale(svtkImageHistogram::Sqrt); }
  svtkGetMacro(HistogramImageScale, int);
  const char* GetHistogramImageScaleAsString();
  //@}

  /**
   * Get the histogram as a svtkIdTypeArray.  You must call Update()
   * before calling this method.
   */
  svtkIdTypeArray* GetHistogram();

  /**
   * Get the total count of the histogram.  This will be the number of
   * voxels times the number of components.
   */
  svtkIdType GetTotal() { return this->Total; }

  /**
   * This is part of the executive, but is public so that it can be accessed
   * by non-member functions.
   */
  void ThreadedRequestData(svtkInformation* request, svtkInformationVector** inputVector,
    svtkInformationVector* outputVector, svtkImageData*** inData, svtkImageData** outData, int ext[6],
    int id) override;

protected:
  svtkImageHistogram();
  ~svtkImageHistogram() override;

  int RequestUpdateExtent(svtkInformation* svtkNotUsed(request), svtkInformationVector** inInfo,
    svtkInformationVector* svtkNotUsed(outInfo)) override;
  int RequestInformation(svtkInformation* svtkNotUsed(request), svtkInformationVector** inInfo,
    svtkInformationVector* svtkNotUsed(outInfo)) override;
  int RequestData(svtkInformation*, svtkInformationVector**, svtkInformationVector*) override;

  int FillInputPortInformation(int port, svtkInformation* info) override;
  int FillOutputPortInformation(int port, svtkInformation* info) override;

  /**
   * Compute the range of the data.  The GetScalarRange() function of
   * svtkImageData only computes the range of the first component, but
   * this filter requires the range for all components.
   */
  void ComputeImageScalarRange(svtkImageData* data, double range[2]);

  int ActiveComponent;
  svtkTypeBool AutomaticBinning;
  int MaximumNumberOfBins;

  int HistogramImageSize[2];
  int HistogramImageScale;
  svtkTypeBool GenerateHistogramImage;

  int NumberOfBins;
  double BinOrigin;
  double BinSpacing;

  svtkIdTypeArray* Histogram;
  svtkIdType Total;

  // Used for svtkMultiThreader operation.
  svtkImageHistogramThreadData* ThreadData;

  // Used for svtkSMPTools operation.
  svtkImageHistogramSMPThreadLocal* SMPThreadData;

private:
  svtkImageHistogram(const svtkImageHistogram&) = delete;
  void operator=(const svtkImageHistogram&) = delete;

  friend class svtkImageHistogramFunctor;
};

#endif
