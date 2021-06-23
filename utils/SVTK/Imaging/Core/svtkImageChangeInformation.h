/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkImageChangeInformation.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   svtkImageChangeInformation
 * @brief   modify spacing, origin and extent.
 *
 * svtkImageChangeInformation  modify the spacing, origin, or extent of
 * the data without changing the data itself.  The data is not resampled
 * by this filter, only the information accompanying the data is modified.
 */

#ifndef svtkImageChangeInformation_h
#define svtkImageChangeInformation_h

#include "svtkImageAlgorithm.h"
#include "svtkImagingCoreModule.h" // For export macro

class svtkImageData;

class SVTKIMAGINGCORE_EXPORT svtkImageChangeInformation : public svtkImageAlgorithm
{
public:
  static svtkImageChangeInformation* New();
  svtkTypeMacro(svtkImageChangeInformation, svtkImageAlgorithm);
  void PrintSelf(ostream& os, svtkIndent indent) override;

  //@{
  /**
   * Copy the information from another data set.  By default,
   * the information is copied from the input.
   */
  virtual void SetInformationInputData(svtkImageData*);
  virtual svtkImageData* GetInformationInput();
  //@}

  //@{
  /**
   * Specify new starting values for the extent explicitly.
   * These values are used as WholeExtent[0], WholeExtent[2] and
   * WholeExtent[4] of the output.  The default is to the
   * use the extent start of the Input, or of the InformationInput
   * if InformationInput is set.
   */
  svtkSetVector3Macro(OutputExtentStart, int);
  svtkGetVector3Macro(OutputExtentStart, int);
  //@}

  //@{
  /**
   * Specify a new data spacing explicitly.  The default is to
   * use the spacing of the Input, or of the InformationInput
   * if InformationInput is set.
   */
  svtkSetVector3Macro(OutputSpacing, double);
  svtkGetVector3Macro(OutputSpacing, double);
  //@}

  //@{
  /**
   * Specify a new data origin explicitly.  The default is to
   * use the origin of the Input, or of the InformationInput
   * if InformationInput is set.
   */
  svtkSetVector3Macro(OutputOrigin, double);
  svtkGetVector3Macro(OutputOrigin, double);
  //@}

  //@{
  /**
   * Set the Origin of the output so that image coordinate (0,0,0)
   * lies at the Center of the data set.  This will override
   * SetOutputOrigin.  This is often a useful operation to apply
   * before using svtkImageReslice to apply a transformation to an image.
   */
  svtkSetMacro(CenterImage, svtkTypeBool);
  svtkBooleanMacro(CenterImage, svtkTypeBool);
  svtkGetMacro(CenterImage, svtkTypeBool);
  //@}

  //@{
  /**
   * Apply a translation to the extent.
   */
  svtkSetVector3Macro(ExtentTranslation, int);
  svtkGetVector3Macro(ExtentTranslation, int);
  //@}

  //@{
  /**
   * Apply a scale factor to the spacing.
   */
  svtkSetVector3Macro(SpacingScale, double);
  svtkGetVector3Macro(SpacingScale, double);
  //@}

  //@{
  /**
   * Apply a translation to the origin.
   */
  svtkSetVector3Macro(OriginTranslation, double);
  svtkGetVector3Macro(OriginTranslation, double);
  //@}

  //@{
  /**
   * Apply a scale to the origin.  The scale is applied
   * before the translation.
   */
  svtkSetVector3Macro(OriginScale, double);
  svtkGetVector3Macro(OriginScale, double);
  //@}

protected:
  svtkImageChangeInformation();
  ~svtkImageChangeInformation() override;

  svtkTypeBool CenterImage;

  int OutputExtentStart[3];
  int ExtentTranslation[3];
  int FinalExtentTranslation[3];

  double OutputSpacing[3];
  double SpacingScale[3];

  double OutputOrigin[3];
  double OriginScale[3];
  double OriginTranslation[3];

  int RequestUpdateExtent(svtkInformation*, svtkInformationVector**, svtkInformationVector*) override;
  int RequestInformation(svtkInformation*, svtkInformationVector**, svtkInformationVector*) override;

  int RequestData(svtkInformation*, svtkInformationVector**, svtkInformationVector*) override;

  int FillInputPortInformation(int port, svtkInformation* info) override;

private:
  svtkImageChangeInformation(const svtkImageChangeInformation&) = delete;
  void operator=(const svtkImageChangeInformation&) = delete;
};

#endif
