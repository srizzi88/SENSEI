/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkWebGLWidget.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   svtkWebGLWidget
 *
 * Widget representation for WebGL.
 */

#ifndef svtkWebGLWidget_h
#define svtkWebGLWidget_h

#include "svtkWebGLExporterModule.h" // needed for export macro
#include "svtkWebGLObject.h"

#include <vector> // Needed to store colors

class svtkActor2D;

class SVTKWEBGLEXPORTER_EXPORT svtkWebGLWidget : public svtkWebGLObject
{
public:
  static svtkWebGLWidget* New();
  svtkTypeMacro(svtkWebGLWidget, svtkWebGLObject);
  void PrintSelf(ostream& os, svtkIndent indent) override;

  void GenerateBinaryData() override;
  unsigned char* GetBinaryData(int part) override;
  int GetBinarySize(int part) override;
  int GetNumberOfParts() override;

  void GetDataFromColorMap(svtkActor2D* actor);

protected:
  svtkWebGLWidget();
  ~svtkWebGLWidget() override;

  unsigned char* binaryData;
  int binarySize;
  int orientation;
  char* title;
  char* textFormat;
  int textPosition;
  float position[2];
  float size[2];
  int numberOfLabels;
  std::vector<double*> colors; // x, r, g, b

private:
  svtkWebGLWidget(const svtkWebGLWidget&) = delete;
  void operator=(const svtkWebGLWidget&) = delete;
};

#endif
