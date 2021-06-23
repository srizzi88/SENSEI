/*=========================================================================

Program:   Visualization Toolkit
Module:    svtkQImageToImageSource.h

Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
All rights reserved.
See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

This software is distributed WITHOUT ANY WARRANTY; without even
the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   svtkQImageToImageSource
 * @brief   Create image data from a QImage.
 *
 * svtkQImageToImageSource produces image data from a QImage.
 */

#ifndef svtkQImageToImageSource_h
#define svtkQImageToImageSource_h

#include "svtkImageAlgorithm.h"
#include "svtkRenderingQtModule.h" // For export macro

class QImage;

class SVTKRENDERINGQT_EXPORT svtkQImageToImageSource : public svtkImageAlgorithm
{
public:
  static svtkQImageToImageSource* New();
  svtkTypeMacro(svtkQImageToImageSource, svtkImageAlgorithm);
  void PrintSelf(ostream& os, svtkIndent indent) override;

  /**
   * Set/Get QImage surface to be used.
   */
  void SetQImage(QImage* image)
  {
    this->QtImage = image;
    this->Modified();
  }
  const QImage* GetQImage() { return QtImage; }

protected:
  svtkQImageToImageSource();
  ~svtkQImageToImageSource() override {}

  const QImage* QtImage;
  int DataExtent[6];

  int RequestData(svtkInformation*, svtkInformationVector**, svtkInformationVector*) override;
  int RequestInformation(svtkInformation* svtkNotUsed(request),
    svtkInformationVector** svtkNotUsed(inputVector), svtkInformationVector* outputVector) override;

private:
  svtkQImageToImageSource(const svtkQImageToImageSource&) = delete;
  void operator=(const svtkQImageToImageSource&) = delete;
};

#endif
