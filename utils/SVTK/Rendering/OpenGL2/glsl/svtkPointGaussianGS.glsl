//SVTK::System::Dec

/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkPointGaussianGS.glsl

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
layout(triangle_strip, max_vertices = 3) out;

uniform int cameraParallel;
uniform float triangleScale;

uniform mat4 VCDCMatrix;

in float radiusVCVSOutput[];
out vec2 offsetVCGSOutput;

// clipping plane vars
//SVTK::Clip::Dec

// picking support
//SVTK::Picking::Dec

void main()
{
  // the offsets sent down are positioned
  // as radius*triangleScale from the center of the
  // gaussian.  This has to be consistent
  // with the offsets we build into the VBO
  float radius = radiusVCVSOutput[0]/triangleScale;

  int i = 0;
  vec4 offset;

  vec4 base1 = vec4(1.0,0.0,0.0,0.0);
  vec4 base2 = vec4(0.0,1.0,0.0,0.0);

  // make the triangle face the camera
  if (cameraParallel == 0)
  {
    vec3 dir = normalize(-gl_in[0].gl_Position.xyz);
    base2 = vec4(normalize(cross(dir,vec3(1.0,0.0,0.0))), 0.0);
    base1 = vec4(cross(base2.xyz,dir),0.0);
  }

  //SVTK::PrimID::Impl

  //SVTK::Clip::Impl

  //SVTK::Color::Impl

  // note 1.73205 = 2.0*cos(30)

  offset = vec4(-1.73205*radiusVCVSOutput[0], -radiusVCVSOutput[0], 0.0, 0.0);

  //SVTK::Picking::Impl

  offsetVCGSOutput = offset.xy/radius;
  gl_Position = VCDCMatrix * (gl_in[0].gl_Position + offset.x*base1 + offset.y*base2);
  EmitVertex();

  offset = vec4(1.73205*radiusVCVSOutput[0], -radiusVCVSOutput[0], 0.0, 0.0);
  offsetVCGSOutput = offset.xy/radius;
  gl_Position = VCDCMatrix * (gl_in[0].gl_Position + offset.x*base1 + offset.y*base2);
  EmitVertex();

  offset = vec4(0.0, 2.0*radiusVCVSOutput[0], 0.0, 0.0);
  offsetVCGSOutput = offset.xy/radius;
  gl_Position = VCDCMatrix * (gl_in[0].gl_Position + offset.x*base1 + offset.y*base2);
  EmitVertex();

  EndPrimitive();
}
