//SVTK::System::Dec

/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkPolyDataVS.glsl

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

in vec4 vertexMC;

//SVTK::CustomUniforms::Dec

// frag position in VC
//SVTK::PositionVC::Dec

// optional normal declaration
//SVTK::Normal::Dec

// extra lighting parameters
//SVTK::Light::Dec

// Texture coordinates
//SVTK::TCoord::Dec

// material property values
//SVTK::Color::Dec

// clipping plane vars
//SVTK::Clip::Dec

// camera and actor matrix values
//SVTK::Camera::Dec

// Apple Bug
//SVTK::PrimID::Dec

// Value raster
//SVTK::ValuePass::Dec

// picking support
//SVTK::Picking::Dec

void main()
{
  //SVTK::Color::Impl

  //SVTK::Normal::Impl

  //SVTK::TCoord::Impl

  //SVTK::Clip::Impl

  //SVTK::PrimID::Impl

  //SVTK::PositionVC::Impl

  //SVTK::ValuePass::Impl

  //SVTK::Light::Impl

  //SVTK::Picking::Impl
}
