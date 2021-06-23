/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkVectorText.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   svtkVectorText
 * @brief   create polygonal text
 *
 *
 * svtkVectorText generates svtkPolyData from an input text string. Besides the
 * ASCII alphanumeric characters a-z, A-Z, 0-9, svtkVectorText also supports
 * ASCII punctuation marks. (The supported ASCII character set are the codes
 * (33-126) inclusive.) The only control character supported is the line feed
 * character "\n", which advances to a new line.
 *
 * To use this class, you normally couple it with a svtkPolyDataMapper and a
 * svtkActor. In this case you would use the svtkActor's transformation methods
 * to position, orient, and scale the text. You may also wish to use a
 * svtkFollower to orient the text so that it always faces the camera.
 *
 * @sa
 * svtkTextMapper svtkCaptionActor2D
 */

#ifndef svtkVectorText_h
#define svtkVectorText_h

#include "svtkPolyDataAlgorithm.h"
#include "svtkRenderingFreeTypeModule.h" // For export macro

class SVTKRENDERINGFREETYPE_EXPORT svtkVectorText : public svtkPolyDataAlgorithm
{
public:
  static svtkVectorText* New();
  svtkTypeMacro(svtkVectorText, svtkPolyDataAlgorithm);
  void PrintSelf(ostream& os, svtkIndent indent) override;

  //@{
  /**
   * Set/Get the text to be drawn.
   */
  svtkSetStringMacro(Text);
  svtkGetStringMacro(Text);
  //@}

protected:
  svtkVectorText();
  ~svtkVectorText() override;

  int RequestData(svtkInformation*, svtkInformationVector**, svtkInformationVector*) override;
  char* Text;

private:
  svtkVectorText(const svtkVectorText&) = delete;
  void operator=(const svtkVectorText&) = delete;
};

#endif
