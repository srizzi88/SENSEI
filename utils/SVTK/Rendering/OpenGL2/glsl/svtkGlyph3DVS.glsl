//SVTK::System::Dec

/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkGlyph3DVS.glsl

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// this shader is used to implement lighting in the fragment shader
// it handles setting up the basic varying variables for the fragment shader

// all variables that represent positions or directions have a suffix
// indicating the coordinate system they are in. The possible values are
// MC - Model Coordinates
// WC - WC world coordinates
// VC - View Coordinates
// DC - Display Coordinates

in vec4 vertexMC;

// frag position in VC
//SVTK::PositionVC::Dec

// optional normal declaration
//SVTK::Normal::Dec

// Texture coordinates
//SVTK::TCoord::Dec

// material property values
//SVTK::Color::Dec

// camera and actor matrix values
//SVTK::Camera::Dec

//SVTK::Glyph::Dec

// clipping plane vars
//SVTK::Clip::Dec

void main()
{
  //SVTK::Glyph::Impl

  //SVTK::Clip::Impl

  //SVTK::Color::Impl

  //SVTK::Normal::Impl

  //SVTK::TCoord::Impl

  // frag position in VC
  //SVTK::PositionVC::Impl
}
