#!/usr/bin/env python
#=========================================================================
#
# Program:   Visualization Toolkit
# Module:    TestProbeFilterImageInput.py
#
# Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
# All rights reserved.
# See Copyright.txt or http:#www.kitware.com/Copyright.htm for details.
#
#    This software is distributed WITHOUT ANY WARRANTY without even
#    the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
#    PURPOSE.  See the above copyright notice for more information.
#
#=========================================================================

import svtk

dim = 48
center = [float(dim)/2.0]*3
extent = [ 0, dim - 1, 0, dim - 1, 0, dim - 1]

imageSource = svtk.svtkRTAnalyticSource()
imageSource.SetWholeExtent(extent[0], extent[1], extent[2], extent[3],
                           extent[4], extent[5])
imageSource.SetCenter(center)
imageSource.Update()

img = imageSource.GetOutput()
scalarRange = img.GetScalarRange()
origin = img.GetOrigin()
spacing = img.GetSpacing()


# create an unstructured grid by generating a point cloud and
# applying Delaunay triangulation on it.
svtk.svtkMath().RandomSeed(0) # svtkPointSource internally uses svtkMath::Random()
pointSource = svtk.svtkPointSource()
pointSource.SetCenter(center)
pointSource.SetRadius(center[0])
pointSource.SetNumberOfPoints(24 * 24 * 24)

delaunay3D = svtk.svtkDelaunay3D()
delaunay3D.SetInputConnection(pointSource.GetOutputPort())

# probe into img using unstructured grif geometry
probe1 = svtk.svtkProbeFilter()
probe1.SetSourceData(img)
probe1.SetInputConnection(delaunay3D.GetOutputPort())

# probe into the unstructured grid using ImageData geometry
outputData = svtk.svtkImageData()
outputData.SetExtent(extent)
outputData.SetOrigin(origin)
outputData.SetSpacing(spacing)
fa = svtk.svtkFloatArray()
fa.SetName("scalars")
fa.Allocate(dim ** 3)
outputData.GetPointData().SetScalars(fa)

probe2 = svtk.svtkProbeFilter()
probe2.SetSourceConnection(probe1.GetOutputPort())
probe2.SetInputData(outputData)


# render using ray-cast volume rendering
ren = svtk.svtkRenderer()
renWin = svtk.svtkRenderWindow()
iren = svtk.svtkRenderWindowInteractor()
renWin.AddRenderer(ren)
iren.SetRenderWindow(renWin)

volumeMapper = svtk.svtkSmartVolumeMapper()
volumeMapper.SetInputConnection(probe2.GetOutputPort())
volumeMapper.SetRequestedRenderModeToRayCast()

volumeColor = svtk.svtkColorTransferFunction()
volumeColor.AddRGBPoint(scalarRange[0], 0.0, 0.0, 1.0)
volumeColor.AddRGBPoint((scalarRange[0] + scalarRange[1]) * 0.5, 0.0, 1.0, 0.0)
volumeColor.AddRGBPoint(scalarRange[1], 1.0, 0.0, 0.0)

volumeScalarOpacity = svtk.svtkPiecewiseFunction()
volumeScalarOpacity.AddPoint(scalarRange[0], 0.0)
volumeScalarOpacity.AddPoint((scalarRange[0] + scalarRange[1]) * 0.5, 0.0)
volumeScalarOpacity.AddPoint(scalarRange[1], 1.0)

volumeProperty = svtk.svtkVolumeProperty()
volumeProperty.SetColor(volumeColor)
volumeProperty.SetScalarOpacity(volumeScalarOpacity)
volumeProperty.SetInterpolationTypeToLinear()
volumeProperty.ShadeOn()
volumeProperty.SetAmbient(0.5)
volumeProperty.SetDiffuse(0.8)
volumeProperty.SetSpecular(0.2)

volume = svtk.svtkVolume()
volume.SetMapper(volumeMapper)
volume.SetProperty(volumeProperty)

ren.AddViewProp(volume)
ren.ResetCamera()
renWin.SetSize(300, 300)
renWin.Render()
