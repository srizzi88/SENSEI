#!/usr/bin/env python

renWin = svtk.svtkRenderWindow()
iren = svtk.svtkRenderWindowInteractor()
iren.SetRenderWindow(renWin)
renderer = svtk.svtkRenderer()
renWin.AddRenderer(renderer)
src1 = svtk.svtkSphereSource()
src1.SetRadius(5)
src1.SetPhiResolution(20)
src1.SetThetaResolution(20)
mapper = svtk.svtkPolyDataMapper()
mapper.SetInputConnection(src1.GetOutputPort())
actor = svtk.svtkActor()
actor.SetMapper(mapper)
# Load the material. Here, we are loading a material
# defined in the Vtk Library. One can also specify
# a filename to a material description xml.
actor.GetProperty().LoadMaterial("GLSLTwisted")
# Turn shading on. Otherwise, shaders are not used.
actor.GetProperty().ShadingOn()
# Pass a shader variable need by GLSLTwisted.
actor.GetProperty().AddShaderVariable("Rate",1.0)
renderer.AddActor(actor)
renWin.Render()
renderer.GetActiveCamera().Azimuth(-50)
renderer.GetActiveCamera().Roll(70)
renWin.Render()
# --- end of script --
