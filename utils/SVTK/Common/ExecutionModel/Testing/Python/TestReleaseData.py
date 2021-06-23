""" Tests if ReleaseData works.
When ReleaseData is set on an algorithm, its output
is supposed to be cleared by the consumer algorithm
once it is done processing it. The following test
verifies this.
"""
import svtk
from svtk.test import Testing

class TestReleaseData(Testing.svtkTest):
    def test(self):
        sphere = svtk.svtkSphereSource()
        sphere.ReleaseDataFlagOn()

        shrink = svtk.svtkShrinkFilter()
        shrink.SetInputConnection(sphere.GetOutputPort())
        shrink.ReleaseDataFlagOn()

        shrink.Update()
        # We expect sphere's output to be wiped by shrink after
        # this update
        self.assertEqual(sphere.GetOutput().GetNumberOfCells(), 0)
        # We update sphere separately. Its output should not be
        # empty
        sphere.Update()
        self.assertTrue(sphere.GetOutput().GetNumberOfCells() > 0)

if __name__ == "__main__":
    Testing.main([(TestReleaseData, 'test')])
