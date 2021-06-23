//SVTK::System::Dec

/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkPointGaussianVS.glsl

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// this shader implements imposters in OpenGL for Spheres

in vec4 vertexMC;
in float radiusMC;
out float radiusVCVSOutput;

// optional normal declaration
//SVTK::Normal::Dec

// Texture coordinates
//SVTK::TCoord::Dec

// material property values
//SVTK::Color::Dec

// clipping plane vars
//SVTK::Clip::Dec

// camera and actor matrix values
//SVTK::Camera::Dec

// picking support
//SVTK::Picking::Dec

void main()
{
  //SVTK::Color::Impl

  //SVTK::Normal::Impl

  //SVTK::TCoord::Impl

  //SVTK::Clip::Impl

  radiusVCVSOutput = radiusMC;

  gl_Position = MCVCMatrix * vertexMC;

  //SVTK::Picking::Impl
}
