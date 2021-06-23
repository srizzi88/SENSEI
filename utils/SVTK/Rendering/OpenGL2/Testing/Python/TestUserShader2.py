#!/usr/bin/env python
'''
=========================================================================

  Program:   Visualization Toolkit
  Module:    TestUserShader2.py

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http:#www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================
'''

import sys
import svtk
from svtk.util.misc import svtkGetDataRoot

'''
  Prevent .pyc files from being created.
  Stops the svtk source being polluted
  by .pyc files.
'''
sys.dont_write_bytecode = True


@svtk.calldata_type(svtk.SVTK_OBJECT)
def svtkShaderCallback(caller, event, calldata):
    program = calldata
    if program is not None:
        diffuseColor = [0.4, 0.7, 0.6]
        program.SetUniform3f("diffuseColorUniform", diffuseColor)


renWin = svtk.svtkRenderWindow()
renWin.SetSize(400, 400)
iren = svtk.svtkRenderWindowInteractor()
iren.SetRenderWindow(renWin)
ren = svtk.svtkRenderer()
ren.SetBackground(0.0, 0.0, 0.0)
ren.GradientBackgroundOn()
renWin.AddRenderer(ren)
actor = svtk.svtkActor()
ren.AddActor(actor)
reader = svtk.svtkPLYReader()
reader.SetFileName("" + str(svtkGetDataRoot()) + "/Data/dragon.ply")
norms = svtk.svtkTriangleMeshPointNormals()
norms.SetInputConnection(reader.GetOutputPort())
mapper = svtk.svtkOpenGLPolyDataMapper()
mapper.SetInputConnection(norms.GetOutputPort())
actor.SetMapper(mapper)
actor.GetProperty().SetAmbientColor(0.2, 0.2, 1.0)
actor.GetProperty().SetDiffuseColor(1.0, 0.65, 0.7)
actor.GetProperty().SetSpecularColor(1.0, 1.0, 1.0)
actor.GetProperty().SetSpecular(0.5)
actor.GetProperty().SetDiffuse(0.7)
actor.GetProperty().SetAmbient(0.5)
actor.GetProperty().SetSpecularPower(20.0)
actor.GetProperty().SetOpacity(1.0)

sp = actor.GetShaderProperty()
sp.SetVertexShaderCode(
    "//SVTK::System::Dec\n"
    "in vec4 vertexMC;\n"
    "//SVTK::Normal::Dec\n"
    "uniform mat4 MCDCMatrix;\n"
    "void main () {\n"
    "  normalVCVSOutput = normalMatrix * normalMC;\n"
    "  vec4 tmpPos = MCDCMatrix * vertexMC;\n"
    "  gl_Position = tmpPos*vec4(0.2+0.8*abs(tmpPos.x),0.2+0.8*abs(tmpPos.y),1.0,1.0);\n"
    "}\n"
)
sp.SetFragmentShaderCode(
    "//SVTK::System::Dec\n"
    "//SVTK::Output::Dec\n"
    "in vec3 normalVCVSOutput;\n"
    "uniform vec3 diffuseColorUniform;\n"
    "void main () {\n"
    "  float df = max(0.0, normalVCVSOutput.z);\n"
    "  float sf = pow(df, 20.0);\n"
    "  vec3 diffuse = df * diffuseColorUniform;\n"
    "  vec3 specular = sf * vec3(0.4,0.4,0.4);\n"
    "  gl_FragData[0] = vec4(0.3*abs(normalVCVSOutput) + 0.7*diffuse + specular, 1.0);\n"
    "}\n"
)

mapper.AddObserver(svtk.svtkCommand.UpdateShaderEvent, svtkShaderCallback)

renWin.Render()
ren.GetActiveCamera().SetPosition(-0.2, 0.4, 1)
ren.GetActiveCamera().SetFocalPoint(0, 0, 0)
ren.GetActiveCamera().SetViewUp(0, 1, 0)
ren.ResetCamera()
ren.GetActiveCamera().Zoom(2.0)
renWin.Render()
iren.Start()
