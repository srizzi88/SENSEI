#!/usr/bin/env python
import svtk
from svtk.util.misc import svtkGetDataRoot
SVTK_DATA_ROOT = svtkGetDataRoot()

r = svtk.svtkExodusIIReader()
r.SetFileName(SVTK_DATA_ROOT + '/Data/can.ex2')

tss = svtk.svtkTemporalShiftScale()
tss.SetInputConnection(r.GetOutputPort())

tss.SetPreShift(.00000001)

st = svtk.svtkSynchronizeTimeFilter()
st.SetRelativeTolerance(.001)
assert st.GetRelativeTolerance() == .001

st.SetInputConnection(tss.GetOutputPort())
st.SetInputConnection(1, r.GetOutputPort())

st.UpdateTimeStep(0)

ds = st.GetOutput()
firstTimeStepValue = ds.GetInformation().Get(svtk.svtkDataObject.DATA_TIME_STEP())

assert firstTimeStepValue == 0.
