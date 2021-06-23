/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkImageItem.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

/**
 * @class   svtkImageItem
 * @brief   a svtkContextItem that draws a supplied image in the
 * scene.
 *
 *
 * This svtkContextItem draws the supplied image in the scene.
 */

#ifndef svtkImageItem_h
#define svtkImageItem_h

#include "svtkContextItem.h"
#include "svtkRenderingContext2DModule.h" // For export macro
#include "svtkSmartPointer.h"             // For SP ivars.

class svtkImageData;

class SVTKRENDERINGCONTEXT2D_EXPORT svtkImageItem : public svtkContextItem
{
public:
  svtkTypeMacro(svtkImageItem, svtkContextItem);
  void PrintSelf(ostream& os, svtkIndent indent) override;

  static svtkImageItem* New();

  /**
   * Paint event for the item.
   */
  bool Paint(svtkContext2D* painter) override;

  /**
   * Set the image of the item.
   */
  void SetImage(svtkImageData* image);

  //@{
  /**
   * Get the image of the item.
   */
  svtkGetObjectMacro(Image, svtkImageData);
  //@}

  //@{
  /**
   * Set the position of the bottom corner of the image.
   */
  svtkSetVector2Macro(Position, float);
  //@}

  //@{
  /**
   * Get the position of the bottom corner of the image.
   */
  svtkGetVector2Macro(Position, float);
  //@}

protected:
  svtkImageItem();
  ~svtkImageItem() override;

  float Position[2];

  svtkImageData* Image;

private:
  svtkImageItem(const svtkImageItem&) = delete;
  void operator=(const svtkImageItem&) = delete;
};

#endif // svtkImageItem_h
