/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkPResampleFilter.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   svtkPResampleFilter
 * @brief   probe dataset in parallel using a svtkImageData
 *
 */

#ifndef svtkPResampleFilter_h
#define svtkPResampleFilter_h

#include "svtkFiltersParallelModule.h" // For export macro
#include "svtkImageAlgorithm.h"

class svtkMultiProcessController;

class SVTKFILTERSPARALLEL_EXPORT svtkPResampleFilter : public svtkImageAlgorithm
{
public:
  svtkTypeMacro(svtkPResampleFilter, svtkImageAlgorithm);
  void PrintSelf(ostream& os, svtkIndent indent) override;

  static svtkPResampleFilter* New();

  //@{
  /**
   * Set and get the controller.
   */
  virtual void SetController(svtkMultiProcessController*);
  svtkGetObjectMacro(Controller, svtkMultiProcessController);
  //@}

  //@{
  /**
   * Set/Get if the filter should use Input bounds to sub-sample the data.
   * By default it is set to 1.
   */
  svtkSetMacro(UseInputBounds, svtkTypeBool);
  svtkGetMacro(UseInputBounds, svtkTypeBool);
  svtkBooleanMacro(UseInputBounds, svtkTypeBool);
  //@}

  //@{
  /**
   * Set/Get sampling bounds. If (UseInputBounds == 1) then the sampling
   * bounds won't be used.
   */
  svtkSetVector6Macro(CustomSamplingBounds, double);
  svtkGetVector6Macro(CustomSamplingBounds, double);
  //@}

  //@{
  /**
   * Set/Get sampling dimension along each axis. Default will be [10,10,10]
   */
  svtkSetVector3Macro(SamplingDimension, int);
  svtkGetVector3Macro(SamplingDimension, int);
  //@}

protected:
  svtkPResampleFilter();
  ~svtkPResampleFilter() override;

  // Usual data generation method
  int RequestData(svtkInformation*, svtkInformationVector**, svtkInformationVector*) override;
  int RequestUpdateExtent(svtkInformation*, svtkInformationVector**, svtkInformationVector*) override;
  int RequestInformation(svtkInformation*, svtkInformationVector**, svtkInformationVector*) override;
  int FillInputPortInformation(int port, svtkInformation* info) override;

  double* CalculateBounds(svtkDataSet* input);

  svtkMultiProcessController* Controller;
  svtkTypeBool UseInputBounds;
  double CustomSamplingBounds[6];
  int SamplingDimension[3];
  double Bounds[6];

private:
  svtkPResampleFilter(const svtkPResampleFilter&) = delete;
  void operator=(const svtkPResampleFilter&) = delete;
};

#endif
