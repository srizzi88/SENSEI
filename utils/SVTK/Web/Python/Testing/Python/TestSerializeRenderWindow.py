import json
import svtk
from svtk.web import render_window_serializer as rws
from svtk.test import Testing

class TestSerializeRenderWindow(Testing.svtkTest):
    def testSerializeRenderWindow(self):
        cone = svtk.svtkConeSource()

        coneMapper = svtk.svtkPolyDataMapper()
        coneMapper.SetInputConnection(cone.GetOutputPort())

        coneActor = svtk.svtkActor()
        coneActor.SetMapper(coneMapper)

        ren = svtk.svtkRenderer()
        ren.AddActor(coneActor)
        renWin = svtk.svtkRenderWindow()
        renWin.AddRenderer(ren)

        ren.ResetCamera()
        renWin.Render()

        # Exercise some of the serialization functionality and make sure it
        # does not generate a stack trace
        context = rws.SynchronizationContext()
        rws.initializeSerializers()
        jsonData = rws.serializeInstance(None, renWin, rws.getReferenceId(renWin), context, 0)

        # jsonStr = json.dumps(jsonData)
        # print jsonStr
        # print len(jsonStr)

if __name__ == "__main__":
    Testing.main([(TestSerializeRenderWindow, 'test')])
