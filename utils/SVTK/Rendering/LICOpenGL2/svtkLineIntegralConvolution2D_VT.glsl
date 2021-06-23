//SVTK::System::Dec

//=========================================================================
//
//  Program:   Visualization Toolkit
//  Module:    svtkLineIntegralConvolution2D_VT.glsl
//
//  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
//  All rights reserved.
//  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.
//
//     This software is distributed WITHOUT ANY WARRANTY; without even
//     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//     PURPOSE.  See the above copyright notice for more information.
//
//=========================================================================

// move vector field to normalized image space
// pre-processing for svtkLineIntegralConvolution2D

// the output of this shader
//SVTK::Output::Dec

// Fragment shader used by the gaussian blur filter render pass.

uniform sampler2D texVectors; // input texture
uniform vec2      uTexSize;   // size of texture

in vec2 tcoordVC;

void main(void)
{
  //SVTK::LICComponentSelection::Impl
  V = V/uTexSize;
  gl_FragData[0] = vec4(V, 0.0, 1.0);
}
