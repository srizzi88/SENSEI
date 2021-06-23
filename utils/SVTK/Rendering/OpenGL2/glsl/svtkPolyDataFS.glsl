//SVTK::System::Dec

/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkPolyDataFS.glsl

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// Template for the polydata mappers fragment shader

uniform int PrimitiveIDOffset;

//SVTK::CustomUniforms::Dec

// VC position of this fragment
//SVTK::PositionVC::Dec

// Camera prop
//SVTK::Camera::Dec

// optional color passed in from the vertex shader, vertexColor
//SVTK::Color::Dec

// optional surface normal declaration
//SVTK::Normal::Dec

// extra lighting parameters
//SVTK::Light::Dec

// Texture maps
//SVTK::TMap::Dec

// Texture coordinates
//SVTK::TCoord::Dec

// picking support
//SVTK::Picking::Dec

// Depth Peeling Support
//SVTK::DepthPeeling::Dec

// clipping plane vars
//SVTK::Clip::Dec

// the output of this shader
//SVTK::Output::Dec

// Apple Bug
//SVTK::PrimID::Dec

// handle coincident offsets
//SVTK::Coincident::Dec

// Value raster
//SVTK::ValuePass::Dec

void main()
{
  // VC position of this fragment. This should not branch/return/discard.
  //SVTK::PositionVC::Impl

  // Place any calls that require uniform flow (e.g. dFdx) here.
  //SVTK::UniformFlow::Impl

  // Set gl_FragDepth here (gl_FragCoord.z by default)
  //SVTK::Depth::Impl

  // Early depth peeling abort:
  //SVTK::DepthPeeling::PreColor

  // Apple Bug
  //SVTK::PrimID::Impl

  //SVTK::Clip::Impl

  //SVTK::ValuePass::Impl

  //SVTK::Color::Impl

  // Generate the normal if we are not passed in one
  //SVTK::Normal::Impl

  //SVTK::Light::Impl

  //SVTK::TCoord::Impl

  if (gl_FragData[0].a <= 0.0)
    {
    discard;
    }

  //SVTK::DepthPeeling::Impl

  //SVTK::Picking::Impl

  // handle coincident offsets
  //SVTK::Coincident::Impl
}
