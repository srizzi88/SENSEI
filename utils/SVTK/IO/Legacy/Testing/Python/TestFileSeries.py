 #!/usr/bin/env python

import os
import svtk
from svtk.util.misc import svtkGetDataRoot

r = svtk.svtkPolyDataReader()
r.AddFileName(svtkGetDataRoot() + "/Data/track1.binary.svtk")
r.AddFileName(svtkGetDataRoot() + "/Data/track2.binary.svtk")
r.AddFileName(svtkGetDataRoot() + "/Data/track3.binary.svtk")
r.UpdateInformation()

assert(r.GetOutputInformation(0).Length(svtk.svtkStreamingDemandDrivenPipeline.TIME_STEPS()) == 3)

r.UpdateTimeStep(1)
rng = r.GetOutput().GetPointData().GetScalars().GetRange(0)
assert(abs(rng[0] - 0.4) < 0.0001)
r.UpdateTimeStep(2)
rng = r.GetOutput().GetPointData().GetScalars().GetRange(0)
assert(abs(rng[1] + 5.8 ) < 0.0001)
