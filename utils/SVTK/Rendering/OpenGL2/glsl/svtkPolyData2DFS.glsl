//SVTK::System::Dec

/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkPolyData2DFS.glsl

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

uniform int PrimitiveIDOffset;

// Texture coordinates
//SVTK::TCoord::Dec

// Scalar coloring
//SVTK::Color::Dec

// Depth Peeling
//SVTK::DepthPeeling::Dec

// picking support
//SVTK::Picking::Dec

// the output of this shader
//SVTK::Output::Dec

// Apple Bug
//SVTK::PrimID::Dec

void main()
{
  // Apple Bug
  //SVTK::PrimID::Impl

  //SVTK::Color::Impl
  //SVTK::TCoord::Impl

  //SVTK::DepthPeeling::Impl
  //SVTK::Picking::Impl

  if (gl_FragData[0].a <= 0.0)
    {
    discard;
    }
}
