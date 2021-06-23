#!/usr/bin/env python
from __future__ import print_function

import svtk
import svtk.test.Testing

class TestStyleRubberBandZoomPerspective(svtk.test.Testing.svtkTest):

    def initPipeline(self):
        try:
            if self.pipelineInitialized:
                # default state
                self.style.LockAspectToViewportOff()
                self.style.CenterAtStartPositionOff()
                self.style.UseDollyForPerspectiveProjectionOn()

                # reset camera too
                self.renderer.ResetCamera()
                self.renderWindow.Render()
        except AttributeError:
            pass

        self.pipelineInitialized = True

        self.sphere = svtk.svtkSphereSource()
        self.idFilter = svtk.svtkIdFilter()
        self.mapper = svtk.svtkPolyDataMapper()
        self.actor = svtk.svtkActor()

        self.idFilter.PointIdsOff()
        self.idFilter.CellIdsOn()
        self.idFilter.SetInputConnection(self.sphere.GetOutputPort())

        self.mapper.SetInputConnection(self.idFilter.GetOutputPort())
        self.mapper.SetColorModeToMapScalars()
        self.mapper.SetScalarModeToUseCellFieldData()
        self.mapper.SelectColorArray("svtkIdFilter_Ids")
        self.mapper.UseLookupTableScalarRangeOff()
        self.mapper.SetScalarRange(0, 95)
        self.actor.SetMapper(self.mapper)

        self.renderer = svtk.svtkRenderer()
        self.renderer.AddActor(self.actor)

        self.renderWindow = svtk.svtkRenderWindow()
        self.renderWindow.AddRenderer(self.renderer)

        self.iren = svtk.svtkRenderWindowInteractor()
        self.iren.SetRenderWindow(self.renderWindow)

        self.style = svtk.svtkInteractorStyleRubberBandZoom()
        self.iren.SetInteractorStyle(self.style)

        self.renderer.GetActiveCamera().SetPosition(0, 0, -1)
        self.renderer.ResetCamera()
        self.renderWindow.Render()

    def interact(self):
        self.iren.SetEventInformationFlipY(150, 150, 0, 0, "0", 0, "0")
        self.iren.InvokeEvent("LeftButtonPressEvent")
        self.iren.SetEventInformationFlipY(192, 182, 0, 0, "0", 0, "0")
        self.iren.InvokeEvent("MouseMoveEvent")
        self.iren.InvokeEvent("LeftButtonReleaseEvent")

    def compare(self, suffix):
        img_file = "TestStyleRubberBandZoomPerspective-%s.png" % suffix
        svtk.test.Testing.compareImage(self.renderWindow,
                svtk.test.Testing.getAbsImagePath(img_file), threshold=25)
        svtk.test.Testing.interact()

    def testDefault(self):
        print("testDefault")
        self.initPipeline()
        self.interact()
        self.compare("Default")

    def testLockAspect(self):
        print("testLockAspect")
        self.initPipeline()
        self.style.LockAspectToViewportOn()
        self.interact()
        self.compare("LockAspect")

    def testCenterAtStartPosition(self):
        print("testCenterAtStartPosition")
        self.initPipeline()
        self.style.CenterAtStartPositionOn()
        self.interact()
        self.compare("CenterAtStartPosition")

    def testCenterAtStartPositionAndLockAspect(self):
        print("testCenterAtStartPositionAndLockAspect")
        self.initPipeline()
        self.style.CenterAtStartPositionOn()
        self.style.LockAspectToViewportOn()
        self.interact()
        self.compare("CenterAtStartPositionAndLockAspect")

    def testParaViewWay(self):
        print("testParaViewWay")
        self.initPipeline()
        self.style.CenterAtStartPositionOn()
        self.style.LockAspectToViewportOn()
        self.style.UseDollyForPerspectiveProjectionOff()
        self.interact()
        self.compare("ParaViewWay")

if __name__ == "__main__":
    svtk.test.Testing.main([(TestStyleRubberBandZoomPerspective, 'test')])
