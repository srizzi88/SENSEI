#!/usr/bin/env python

# This tests svtkTemporalSnapToTimeStep

import svtk
from svtk.test import Testing
from svtk.util.misc import svtkGetDataRoot

SVTK_DATA_ROOT = svtkGetDataRoot()

def Nearest(a):
  i = int(a)
  if a-i <= 0.5:
    return i
  else:
    return i+1;

class TestTemporalSnapToTimeStep(Testing.svtkTest):
  def test(self):
    source = svtk.svtkTemporalFractal()
    source.DiscreteTimeStepsOn();

    shift = svtk.svtkTemporalSnapToTimeStep()
    shift.SetInputConnection(source.GetOutputPort())

    for i in range(4):
      inTime = i*0.5+0.1
      shift.UpdateTimeStep(inTime)
      self.assertEqual(shift.GetOutputDataObject(0).GetInformation().Has(svtk.svtkDataObject.DATA_TIME_STEP()),True)
      outTime = shift.GetOutputDataObject(0).GetInformation().Get(svtk.svtkDataObject.DATA_TIME_STEP())
      self.assertEqual(outTime==Nearest(inTime),True);


if __name__ == "__main__":
    Testing.main([(TestTemporalSnapToTimeStep, 'test')])
