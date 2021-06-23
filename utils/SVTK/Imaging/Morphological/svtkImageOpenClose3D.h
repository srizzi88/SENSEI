/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkImageOpenClose3D.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   svtkImageOpenClose3D
 * @brief   Will perform opening or closing.
 *
 * svtkImageOpenClose3D performs opening or closing by having two
 * svtkImageErodeDilates in series.  The size of operation
 * is determined by the method SetKernelSize, and the operator is an ellipse.
 * OpenValue and CloseValue determine how the filter behaves.  For binary
 * images Opening and closing behaves as expected.
 * Close value is first dilated, and then eroded.
 * Open value is first eroded, and then dilated.
 * Degenerate two dimensional opening/closing can be achieved by setting the
 * one axis the 3D KernelSize to 1.
 * Values other than open value and close value are not touched.
 * This enables the filter to processes segmented images containing more than
 * two tags.
 */

#ifndef svtkImageOpenClose3D_h
#define svtkImageOpenClose3D_h

#include "svtkImageAlgorithm.h"
#include "svtkImagingMorphologicalModule.h" // For export macro

class svtkImageDilateErode3D;

class SVTKIMAGINGMORPHOLOGICAL_EXPORT svtkImageOpenClose3D : public svtkImageAlgorithm
{
public:
  //@{
  /**
   * Default open value is 0, and default close value is 255.
   */
  static svtkImageOpenClose3D* New();
  svtkTypeMacro(svtkImageOpenClose3D, svtkImageAlgorithm);
  void PrintSelf(ostream& os, svtkIndent indent) override;
  //@}

  /**
   * This method considers the sub filters MTimes when computing this objects
   * modified time.
   */
  svtkMTimeType GetMTime() override;

  //@{
  /**
   * Turn debugging output on. (in sub filters also)
   */
  void DebugOn() override;
  void DebugOff() override;
  //@}

  /**
   * Pass modified message to sub filters.
   */
  void Modified() override;

  // Forward Source messages to filter1

  /**
   * Selects the size of gaps or objects removed.
   */
  void SetKernelSize(int size0, int size1, int size2);

  //@{
  /**
   * Determines the value that will opened.
   * Open value is first eroded, and then dilated.
   */
  void SetOpenValue(double value);
  double GetOpenValue();
  //@}

  //@{
  /**
   * Determines the value that will closed.
   * Close value is first dilated, and then eroded
   */
  void SetCloseValue(double value);
  double GetCloseValue();
  //@}

  //@{
  /**
   * Needed for Progress functions
   */
  svtkGetObjectMacro(Filter0, svtkImageDilateErode3D);
  svtkGetObjectMacro(Filter1, svtkImageDilateErode3D);
  //@}

  /**
   * see svtkAlgorithm for details
   */
  svtkTypeBool ProcessRequest(
    svtkInformation*, svtkInformationVector**, svtkInformationVector*) override;

  /**
   * Override to send the request to internal pipeline.
   */
  int ComputePipelineMTime(svtkInformation* request, svtkInformationVector** inInfoVec,
    svtkInformationVector* outInfoVec, int requestFromOutputPort, svtkMTimeType* mtime) override;

protected:
  svtkImageOpenClose3D();
  ~svtkImageOpenClose3D() override;

  svtkImageDilateErode3D* Filter0;
  svtkImageDilateErode3D* Filter1;

  void ReportReferences(svtkGarbageCollector*) override;

private:
  svtkImageOpenClose3D(const svtkImageOpenClose3D&) = delete;
  void operator=(const svtkImageOpenClose3D&) = delete;
};

#endif
