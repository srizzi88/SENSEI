/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkRTAnalyticSource.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   svtkRTAnalyticSource
 * @brief   Create an image for regression testing
 *
 * svtkRTAnalyticSource just produces images with pixel values determined
 * by a Maximum*Gaussian*XMag*sin(XFreq*x)*sin(YFreq*y)*cos(ZFreq*z)
 * Values are float scalars on point data with name "RTData".
 */

#ifndef svtkRTAnalyticSource_h
#define svtkRTAnalyticSource_h

#include "svtkImageAlgorithm.h"
#include "svtkImagingCoreModule.h" // For export macro

class SVTKIMAGINGCORE_EXPORT svtkRTAnalyticSource : public svtkImageAlgorithm
{
public:
  static svtkRTAnalyticSource* New();
  svtkTypeMacro(svtkRTAnalyticSource, svtkImageAlgorithm);
  void PrintSelf(ostream& os, svtkIndent indent) override;

  //@{
  /**
   * Set/Get the extent of the whole output image. Initial value is
   * {-10,10,-10,10,-10,10}
   */
  void SetWholeExtent(int xMinx, int xMax, int yMin, int yMax, int zMin, int zMax);
  svtkGetVector6Macro(WholeExtent, int);
  //@}

  //@{
  /**
   * Set/Get the center of function. Initial value is {0.0,0.0,0.0}
   */
  svtkSetVector3Macro(Center, double);
  svtkGetVector3Macro(Center, double);
  //@}

  //@{
  /**
   * Set/Get the Maximum value of the function. Initial value is 255.0.
   */
  svtkSetMacro(Maximum, double);
  svtkGetMacro(Maximum, double);
  //@}

  //@{
  /**
   * Set/Get the standard deviation of the function. Initial value is 0.5.
   */
  svtkSetMacro(StandardDeviation, double);
  svtkGetMacro(StandardDeviation, double);
  //@}

  //@{
  /**
   * Set/Get the natural frequency in x. Initial value is 60.
   */
  svtkSetMacro(XFreq, double);
  svtkGetMacro(XFreq, double);
  //@}

  //@{
  /**
   * Set/Get the natural frequency in y. Initial value is 30.
   */
  svtkSetMacro(YFreq, double);
  svtkGetMacro(YFreq, double);
  //@}

  //@{
  /**
   * Set/Get the natural frequency in z. Initial value is 40.
   */
  svtkSetMacro(ZFreq, double);
  svtkGetMacro(ZFreq, double);
  //@}

  //@{
  /**
   * Set/Get the magnitude in x. Initial value is 10.
   */
  svtkSetMacro(XMag, double);
  svtkGetMacro(XMag, double);
  //@}

  //@{
  /**
   * Set/Get the magnitude in y. Initial value is 18.
   */
  svtkSetMacro(YMag, double);
  svtkGetMacro(YMag, double);
  //@}

  //@{
  /**
   * Set/Get the magnitude in z. Initial value is 5.
   */
  svtkSetMacro(ZMag, double);
  svtkGetMacro(ZMag, double);
  //@}

  //@{
  /**
   * Set/Get the sub-sample rate. Initial value is 1.
   */
  svtkSetMacro(SubsampleRate, int);
  svtkGetMacro(SubsampleRate, int);
  //@}

protected:
  /**
   * Default constructor. Initial values are:
   * Maximum=255.0, Center[3]={0.0,0.0,0.0}, WholeExtent={-10,10,-10,10,-10,10}
   * StandardDeviation=0.5, XFreq=60, XMag=10, YFreq=30, YMag=18, ZFreq=40,
   * ZMag=5, SubsampleRate=1
   */
  svtkRTAnalyticSource();

  /**
   * Destructor.
   */
  ~svtkRTAnalyticSource() override {}

  double XFreq;
  double YFreq;
  double ZFreq;
  double XMag;
  double YMag;
  double ZMag;
  double StandardDeviation;
  int WholeExtent[6];
  double Center[3];
  double Maximum;
  int SubsampleRate;

  int RequestInformation(svtkInformation* request, svtkInformationVector** inputVector,
    svtkInformationVector* outputVector) override;
  void ExecuteDataWithInformation(svtkDataObject* data, svtkInformation* outInfo) override;

  int FillOutputPortInformation(int port, svtkInformation* info) override;

private:
  svtkRTAnalyticSource(const svtkRTAnalyticSource&) = delete;
  void operator=(const svtkRTAnalyticSource&) = delete;
};

#endif
