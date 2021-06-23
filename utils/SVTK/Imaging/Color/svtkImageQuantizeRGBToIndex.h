/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkImageQuantizeRGBToIndex.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   svtkImageQuantizeRGBToIndex
 * @brief   generalized histograms up to 4 dimensions
 *
 * svtkImageQuantizeRGBToIndex takes a 3 component RGB image as
 * input and produces a one component index image as output, along with
 * a lookup table that contains the color definitions for the index values.
 * This filter works on the entire input extent - it does not perform
 * streaming, and it does not supported threaded execution (because it has
 * to process the entire image).
 *
 * To use this filter, you typically set the number of colors
 * (between 2 and 65536), execute it, and then retrieve the lookup table.
 * The colors can then be using the lookup table and the image index.
 *
 * This filter can run faster, by initially sampling the colors at a
 * coarser level. This can be specified by the SamplingRate parameter.
 *
 * The "index-image" viewed as a greyscale image, is usually quite
 * arbitrary, accentuating contrast where none can be perceived in
 * the original color image.
 * To make the index image more meaningful (e.g. for image segmentation
 * operating on scalar images), we sort the mean colors by luminance
 * and re-map the indices accordingly. This option does not introduce any
 * computational complexity and has no impact on actual colors in the
 * lookup table (only their order).
 */

#ifndef svtkImageQuantizeRGBToIndex_h
#define svtkImageQuantizeRGBToIndex_h

#include "svtkImageAlgorithm.h"
#include "svtkImagingColorModule.h" // For export macro

class svtkLookupTable;

class SVTKIMAGINGCOLOR_EXPORT svtkImageQuantizeRGBToIndex : public svtkImageAlgorithm
{
public:
  static svtkImageQuantizeRGBToIndex* New();
  svtkTypeMacro(svtkImageQuantizeRGBToIndex, svtkImageAlgorithm);
  void PrintSelf(ostream& os, svtkIndent indent) override;

  //@{
  /**
   * Set / Get the number of color index values to produce - must be
   * a number between 2 and 65536.
   */
  svtkSetClampMacro(NumberOfColors, int, 2, 65536);
  svtkGetMacro(NumberOfColors, int);
  //@}

  svtkSetVector3Macro(SamplingRate, int);
  svtkGetVector3Macro(SamplingRate, int);

  svtkSetMacro(SortIndexByLuminance, bool);
  svtkGetMacro(SortIndexByLuminance, bool);
  svtkBooleanMacro(SortIndexByLuminance, bool);

  //@{
  /**
   * Get the resulting lookup table that contains the color definitions
   * corresponding to the index values in the output image.
   */
  svtkGetObjectMacro(LookupTable, svtkLookupTable);
  //@}

  svtkGetMacro(InitializeExecuteTime, double);
  svtkGetMacro(BuildTreeExecuteTime, double);
  svtkGetMacro(LookupIndexExecuteTime, double);

  //@{
  /**
   * For internal use only - get the type of the image
   */
  svtkGetMacro(InputType, int);
  //@}

  //@{
  /**
   * For internal use only - set the times for execution
   */
  svtkSetMacro(InitializeExecuteTime, double);
  svtkSetMacro(BuildTreeExecuteTime, double);
  svtkSetMacro(LookupIndexExecuteTime, double);
  //@}

protected:
  svtkImageQuantizeRGBToIndex();
  ~svtkImageQuantizeRGBToIndex() override;

  svtkLookupTable* LookupTable;
  int NumberOfColors;
  int InputType;
  int SamplingRate[3];
  bool SortIndexByLuminance;

  double InitializeExecuteTime;
  double BuildTreeExecuteTime;
  double LookupIndexExecuteTime;

  int RequestInformation(svtkInformation*, svtkInformationVector**, svtkInformationVector*) override;
  int RequestUpdateExtent(svtkInformation*, svtkInformationVector**, svtkInformationVector*) override;

  int RequestData(svtkInformation*, svtkInformationVector**, svtkInformationVector*) override;

private:
  svtkImageQuantizeRGBToIndex(const svtkImageQuantizeRGBToIndex&) = delete;
  void operator=(const svtkImageQuantizeRGBToIndex&) = delete;
};

#endif
