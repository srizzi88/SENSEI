//SVTK::System::Dec

/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkStickMapperGS.glsl

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// Template for the polydata mappers geometry shader

// primitiveID
//SVTK::PrimID::Dec

// optional color passed in from the vertex shader, vertexColor
//SVTK::Color::Dec

layout(points) in;
layout(triangle_strip, max_vertices = 6) out;

uniform int cameraParallel;

uniform mat4 VCDCMatrix;

in float radiusVCVSOutput[];
out float radiusVCGSOutput;

out vec4 vertexVCGSOutput;

in float lengthVCVSOutput[];
out float lengthVCGSOutput;

out vec3 centerVCGSOutput;

in vec3 orientVCVSOutput[];
out vec3 orientVCGSOutput;

// clipping plane vars
//SVTK::Clip::Dec

//SVTK::Picking::Dec

void main()
{
  centerVCGSOutput = gl_in[0].gl_Position.xyz/gl_in[0].gl_Position.w;
  radiusVCGSOutput = radiusVCVSOutput[0];
  lengthVCGSOutput = lengthVCVSOutput[0];
  orientVCGSOutput = orientVCVSOutput[0];

  int i = 0;

  //SVTK::PrimID::Impl

  //SVTK::Clip::Impl

  //SVTK::Color::Impl

  //SVTK::Picking::Impl

  // make the basis
  vec3 xbase;
  vec3 ybase;

  // dir is the direction to the camera
  vec3 dir = vec3(0.0,0.0,1.0);
  if (cameraParallel == 0)
  {
    dir = normalize(-centerVCGSOutput);
  }

  // if dir is aligned with the cylinder orientation
  if (abs(dot(dir,orientVCGSOutput)) == 1.0)
  {
    xbase = normalize(cross(vec3(0.0,1.0,0.0),orientVCGSOutput));
    ybase = cross(xbase,orientVCGSOutput);
  }
  else
  {
    xbase = normalize(cross(orientVCGSOutput,dir));
    ybase = cross(orientVCGSOutput,xbase);
  }
  xbase = xbase * radiusVCGSOutput;
  ybase = ybase * radiusVCGSOutput;
  vec3 zbase = 0.5*lengthVCGSOutput*orientVCGSOutput;

  vertexVCGSOutput = vec4(0.0, 0.0, 0.0, 1.0);
  vertexVCGSOutput.xyz = centerVCGSOutput
    - xbase - ybase - zbase;
  gl_Position = VCDCMatrix * vertexVCGSOutput;
  EmitVertex();

  vertexVCGSOutput.xyz = centerVCGSOutput
    + xbase - ybase - zbase;
  gl_Position = VCDCMatrix * vertexVCGSOutput;
  EmitVertex();

  vertexVCGSOutput.xyz = centerVCGSOutput
    - xbase - ybase + zbase;
  gl_Position = VCDCMatrix * vertexVCGSOutput;
  EmitVertex();

  vertexVCGSOutput.xyz = centerVCGSOutput
    + xbase - ybase + zbase;
  gl_Position = VCDCMatrix * vertexVCGSOutput;
  EmitVertex();

  vertexVCGSOutput.xyz = centerVCGSOutput
    - xbase + ybase + zbase;
  gl_Position = VCDCMatrix * vertexVCGSOutput;
  EmitVertex();

  vertexVCGSOutput.xyz = centerVCGSOutput
    + xbase + ybase + zbase;
  gl_Position = VCDCMatrix * vertexVCGSOutput;
  EmitVertex();

  EndPrimitive();
}
