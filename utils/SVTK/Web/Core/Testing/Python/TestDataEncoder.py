import sys
import svtk
import array
from svtk.test import Testing


class TestDataEncoder(Testing.svtkTest):
    def testEncodings(self):
        # Render something
        cylinder = svtk.svtkCylinderSource()
        cylinder.SetResolution(8)

        cylinderMapper = svtk.svtkPolyDataMapper()
        cylinderMapper.SetInputConnection(cylinder.GetOutputPort())

        cylinderActor = svtk.svtkActor()
        cylinderActor.SetMapper(cylinderMapper)
        cylinderActor.RotateX(30.0)
        cylinderActor.RotateY(-45.0)

        ren = svtk.svtkRenderer()
        renWin = svtk.svtkRenderWindow()
        renWin.AddRenderer(ren)
        ren.AddActor(cylinderActor)
        renWin.SetSize(200, 200)

        ren.ResetCamera()
        ren.GetActiveCamera().Zoom(1.5)
        renWin.Render()

        # Get a svtkImageData with the rendered output
        w2if = svtk.svtkWindowToImageFilter()
        w2if.SetInput(renWin)
        w2if.SetShouldRerender(1)
        w2if.SetReadFrontBuffer(0)
        w2if.Update()
        imgData = w2if.GetOutput()

        # Use svtkDataEncoder to convert the image to PNG format and Base64 encode it
        encoder = svtk.svtkDataEncoder()
        base64String = encoder.EncodeAsBase64Png(imgData).encode('ascii')

        # Now Base64 decode the string back to PNG image data bytes
        inputArray = array.array('B', base64String)
        outputBuffer = bytearray(len(inputArray))

        utils = None
        try:
            utils = svtk.svtkIOCore.svtkBase64Utilities()
        except:
            try:
                utils = svtkIOCore.svtkBase64Utilities()
            except:
                print('Unable to import required svtkIOCore.svtkBase64Utilities')
                return

        actualLength = utils.DecodeSafely(inputArray, len(inputArray), outputBuffer, len(outputBuffer))
        outputArray = bytearray(actualLength)
        outputArray[:] = outputBuffer[0:actualLength]

        # And write those bytes to the disk as an actual PNG image file
        with open('TestDataEncoder.png', 'wb') as fd:
            fd.write(outputArray)

        # Create a svtkTesting object and specify a baseline image
        rtTester = svtk.svtkTesting()
        for arg in sys.argv[1:]:
            rtTester.AddArgument(arg)
        rtTester.AddArgument("-V")
        rtTester.AddArgument("TestDataEncoder.png")

        # Perform the image comparison test and print out the result.
        result = rtTester.RegressionTest("TestDataEncoder.png", 0.0)

        if result == 0:
            raise Exception("TestDataEncoder failed.")

if __name__ == "__main__":
    Testing.main([(TestDataEncoder, 'test')])
