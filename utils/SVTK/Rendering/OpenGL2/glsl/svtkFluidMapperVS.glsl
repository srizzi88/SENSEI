//SVTK::System::Dec

/*=========================================================================

  Program:   Visualization Toolkit

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// this shader implements fluid imposters in OpenGL as Spheres

in vec4 vertexMC;
in vec3 vertexColor;

uniform int   hasVertexColor   = 0;

// optional normal declaration
//SVTK::Normal::Dec

// Texture coordinates
//SVTK::TCoord::Dec

// material property values
//SVTK::Color::Dec

// clipping plane vars
//SVTK::Clip::Dec

// camera and actor matrix values
uniform mat4 MCVCMatrix;

// picking support
//SVTK::Picking::Dec

// Pass vertex color to fragment shader
out vec3 colorVSOut;

void main() {
    //SVTK::Color::Impl

    //SVTK::Normal::Impl

    //SVTK::TCoord::Impl

    //SVTK::Clip::Impl

    gl_Position = MCVCMatrix * vertexMC;

    if(hasVertexColor == 1) {
        colorVSOut = vertexColor;
    }

    //SVTK::Picking::Impl
}
