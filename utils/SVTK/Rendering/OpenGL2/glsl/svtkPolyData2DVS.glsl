//SVTK::System::Dec

/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkPolyData2DVS.glsl

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

// all variables that represent positions or directions have a suffix
// indicating the coordinate system they are in. The possible values are
// MC - Model Coordinates
// WC - WC world coordinates
// VC - View Coordinates
// DC - Display Coordinates

in vec4 vertexWC;

// material property values
//SVTK::Color::Dec

// Texture coordinates
//SVTK::TCoord::Dec

// Apple Bug
//SVTK::PrimID::Dec

uniform mat4 WCVCMatrix;  // World to view matrix

void main()
{
  // Apple Bug
  //SVTK::PrimID::Impl

  gl_Position = WCVCMatrix*vertexWC;
  //SVTK::TCoord::Impl
  //SVTK::Color::Impl
}
