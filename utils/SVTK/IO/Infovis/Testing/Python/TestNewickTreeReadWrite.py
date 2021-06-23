import sys
import svtk
from svtk.test import Testing

class TestNewickTreeReadWrite(Testing.svtkTest):

    def testReadWrite(self):
        t = '(A:1,B:2,(C:3,D:4)E:5)F;'

        reader = svtk.svtkNewickTreeReader()
        reader.SetReadFromInputString(True)
        reader.SetInputString(t)
        reader.Update()

        writer = svtk.svtkNewickTreeWriter()
        writer.WriteToOutputStringOn()
        writer.SetInputData(reader.GetOutput())
        writer.Update()
        t_return = writer.GetOutputStdString()

        self.assertEqual(t,t_return)
