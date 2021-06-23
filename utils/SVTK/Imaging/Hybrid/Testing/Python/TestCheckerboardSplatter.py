
import svtk
import svtk.test.Testing
from svtk.util.misc import svtkGetDataRoot
SVTK_DATA_ROOT = svtkGetDataRoot()

# Slat points into a cube
math = svtk.svtkMath()
points = svtk.svtkPoints()
i = 0
while i < 100000:
    points.InsertPoint(i,math.Random(0,1),math.Random(0,1),math.Random(0,1))
    i = i + 1

profile = svtk.svtkPolyData()
profile.SetPoints(points)

# Checkerboard
cbdSplatter = svtk.svtkCheckerboardSplatter()
cbdSplatter.SetInputData(profile)
cbdSplatter.SetSampleDimensions(100, 100, 100)
cbdSplatter.ScalarWarpingOff()
cbdSplatter.SetFootprint(2)
cbdSplatter.SetParallelSplatCrossover(2)
#cbdSplatter.SetRadius(0.05)

cbdSurface = svtk.svtkMarchingContourFilter()
cbdSurface.SetInputConnection(cbdSplatter.GetOutputPort())
cbdSurface.SetValue(0, 0.01)

cbdMapper = svtk.svtkPolyDataMapper()
cbdMapper.SetInputConnection(cbdSurface.GetOutputPort())
cbdMapper.ScalarVisibilityOff()

cbdActor = svtk.svtkActor()
cbdActor.SetMapper(cbdMapper)
cbdActor.GetProperty().SetColor(1.0, 0.0, 0.0)

timer = svtk.svtkExecutionTimer()
timer.SetFilter(cbdSplatter)
cbdSplatter.Update()
CBDwallClock = timer.GetElapsedWallClockTime()
#print ("CBDSplat:", CBDwallClock)

# Graphics stuff
#
ren = svtk.svtkRenderer()
renWin = svtk.svtkRenderWindow()
renWin.AddRenderer(ren)

camera = svtk.svtkCamera()
camera.SetFocalPoint(0,0,0)
camera.SetPosition(1,1,1)
ren.SetActiveCamera(camera)

# Add the actors to the renderer, set the background and size
#
ren.AddActor(cbdActor)
ren.SetBackground(1, 1, 1)
renWin.SetSize(400, 400)
ren.ResetCamera()

# render and interact with data
iRen = svtk.svtkRenderWindowInteractor()
iRen.SetRenderWindow(renWin);
renWin.Render()

#iRen.Start()

# prevent the tk window from showing up then start the event loop
# --- end of script --
