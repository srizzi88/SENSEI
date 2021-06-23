/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkResampleToImage.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   svtkResampleToImage
 * @brief   sample dataset on a uniform grid
 *
 * svtkPResampleToImage is a filter that resamples the input dataset on
 * a uniform grid. It internally uses svtkProbeFilter to do the probing.
 * @sa
 * svtkProbeFilter
 */

#ifndef svtkResampleToImage_h
#define svtkResampleToImage_h

#include "svtkAlgorithm.h"
#include "svtkFiltersCoreModule.h" // For export macro
#include "svtkNew.h"               // For svtkCompositeDataProbeFilter member variable

class svtkDataObject;
class svtkImageData;

class SVTKFILTERSCORE_EXPORT svtkResampleToImage : public svtkAlgorithm
{
public:
  svtkTypeMacro(svtkResampleToImage, svtkAlgorithm);
  void PrintSelf(ostream& os, svtkIndent indent) override;

  static svtkResampleToImage* New();

  //@{
  /**
   * Set/Get if the filter should use Input bounds to sub-sample the data.
   * By default it is set to 1.
   */
  svtkSetMacro(UseInputBounds, bool);
  svtkGetMacro(UseInputBounds, bool);
  svtkBooleanMacro(UseInputBounds, bool);
  //@}

  //@{
  /**
   * Set/Get sampling bounds. If (UseInputBounds == 1) then the sampling
   * bounds won't be used.
   */
  svtkSetVector6Macro(SamplingBounds, double);
  svtkGetVector6Macro(SamplingBounds, double);
  //@}

  //@{
  /**
   * Set/Get sampling dimension along each axis. Default will be [10,10,10]
   */
  svtkSetVector3Macro(SamplingDimensions, int);
  svtkGetVector3Macro(SamplingDimensions, int);
  //@}

  /**
   * Get the output data for this algorithm.
   */
  svtkImageData* GetOutput();

protected:
  svtkResampleToImage();
  ~svtkResampleToImage() override;

  // Usual data generation method
  svtkTypeBool ProcessRequest(
    svtkInformation*, svtkInformationVector**, svtkInformationVector*) override;
  virtual int RequestData(svtkInformation*, svtkInformationVector**, svtkInformationVector*);
  virtual int RequestInformation(svtkInformation*, svtkInformationVector**, svtkInformationVector*);
  virtual int RequestUpdateExtent(svtkInformation*, svtkInformationVector**, svtkInformationVector*);
  int FillInputPortInformation(int, svtkInformation*) override;
  int FillOutputPortInformation(int, svtkInformation*) override;

  /**
   * Get the name of the valid-points mask array.
   */
  const char* GetMaskArrayName() const;

  /**
   * Resample input svtkDataObject to a svtkImageData with the specified bounds
   * and extent.
   */
  void PerformResampling(svtkDataObject* input, const double samplingBounds[6],
    bool computeProbingExtent, const double inputBounds[6], svtkImageData* output);

  /**
   * Mark invalid points and cells of svtkImageData as hidden
   */
  void SetBlankPointsAndCells(svtkImageData* data);

  /**
   * Helper function to compute the bounds of the given svtkDataSet or
   * svtkCompositeDataSet
   */
  static void ComputeDataBounds(svtkDataObject* data, double bounds[6]);

  bool UseInputBounds;
  double SamplingBounds[6];
  int SamplingDimensions[3];

private:
  svtkResampleToImage(const svtkResampleToImage&) = delete;
  void operator=(const svtkResampleToImage&) = delete;
};

#endif
