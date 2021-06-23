//SVTK::System::Dec

/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkPolyDataWideLineGS.glsl

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// Template for the polydata mappers geometry shader

// VC position of this fragment
//SVTK::PositionVC::Dec

// primitiveID
//SVTK::PrimID::Dec

// optional color passed in from the vertex shader, vertexColor
//SVTK::Color::Dec

// optional surface normal declaration
//SVTK::Normal::Dec

// extra lighting parameters
//SVTK::Light::Dec

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

uniform vec2 lineWidthNVC;

layout(lines) in;
layout(triangle_strip, max_vertices = 4) out;

void main()
{
  // compute the lines direction
  vec2 normal = normalize(
    gl_in[1].gl_Position.xy/gl_in[1].gl_Position.w -
    gl_in[0].gl_Position.xy/gl_in[0].gl_Position.w);

  // rotate 90 degrees
  normal = vec2(-1.0*normal.y,normal.x);

  //SVTK::Normal::Start

  for (int j = 0; j < 4; j++)
    {
    int i = j/2;

    //SVTK::PrimID::Impl

    //SVTK::Clip::Impl

    //SVTK::Color::Impl

    //SVTK::Normal::Impl

    //SVTK::Light::Impl

    //SVTK::TCoord::Impl

    //SVTK::DepthPeeling::Impl

    //SVTK::Picking::Impl

    // VC position of this fragment
    //SVTK::PositionVC::Impl

    gl_Position = vec4(
      gl_in[i].gl_Position.xy + (lineWidthNVC*normal)*((j+1)%2 - 0.5)*gl_in[i].gl_Position.w,
      gl_in[i].gl_Position.z,
      gl_in[i].gl_Position.w);
    EmitVertex();
    }
  EndPrimitive();
}
