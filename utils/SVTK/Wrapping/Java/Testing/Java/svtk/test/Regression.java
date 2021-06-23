package svtk.test;

/*=========================================================================

 Program:   Visualization Toolkit
 Module:    Regression.java

 Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
 All rights reserved.
 See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

 This software is distributed WITHOUT ANY WARRANTY; without even
 the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
 PURPOSE.  See the above copyright notice for more information.

 =========================================================================*/
import javax.swing.SwingUtilities;

import svtk.svtkActor;
import svtk.svtkConeSource;
import svtk.svtkImageData;
import svtk.svtkImageDifference;
import svtk.svtkJavaTesting;
import svtk.svtkObject;
import svtk.svtkPNGWriter;
import svtk.svtkPolyDataMapper;
import svtk.svtkRenderWindow;
import svtk.svtkRenderWindowInteractor;
import svtk.svtkRenderer;
import svtk.svtkShortArray;
import svtk.svtkUnsignedCharArray;
import svtk.svtkUnsignedShortArray;
import svtk.svtkWindowToImageFilter;

public class Regression {

  public static void main(final String[] args) {
    SwingUtilities.invokeLater(new Runnable() {
      public void run() {
        svtkJavaTesting.Initialize(args, true);
        svtkShortArray array = new svtkShortArray();
        array.InsertNextTuple1(3.0);
        array.InsertNextTuple1(1.0);
        array.InsertNextTuple1(4.0);
        array.InsertNextTuple1(1.0);
        array.InsertNextTuple1(5.0);
        array.InsertNextTuple1(9.0);
        array.InsertNextTuple1(2.0);
        array.InsertNextTuple1(6.0);
        array.InsertNextTuple1(5.0);
        array.InsertNextTuple1(3.0);
        array.InsertNextTuple1(5.0);
        array.InsertNextTuple1(8.0);
        array.InsertNextTuple1(9.0);
        array.InsertNextTuple1(7.0);
        array.InsertNextTuple1(9.0);
        array.InsertNextTuple1(3.0);
        array.InsertNextTuple1(1.0);
        short[] carray = array.GetJavaArray();
        int cc;
        System.out.print("[");
        for (cc = 0; cc < carray.length; cc++) {
          short i = carray[cc];
          System.out.print(i);
        }
        System.out.println("]");

        svtkUnsignedShortArray narray = new svtkUnsignedShortArray();
        narray.SetJavaArray(carray);
        System.out.print("[");
        for (cc = 0; cc <= narray.GetMaxId(); cc++) {
          int i = narray.GetValue(cc);
          System.out.print(i);
        }
        System.out.println("]");

        svtkRenderWindow renWin = new svtkRenderWindow();
        svtkRenderer ren1 = new svtkRenderer();
        renWin.AddRenderer(ren1);
        svtkRenderWindowInteractor iren = new svtkRenderWindowInteractor();
        iren.SetRenderWindow(renWin);
        svtkConeSource cone = new svtkConeSource();
        cone.SetResolution(8);
        svtkPolyDataMapper coneMapper = new svtkPolyDataMapper();
        coneMapper.SetInputConnection(cone.GetOutputPort());

        svtkActor coneActor = new svtkActor();
        coneActor.SetMapper(coneMapper);

        ren1.AddActor(coneActor);
        renWin.Render();
        svtkWindowToImageFilter w2i = new svtkWindowToImageFilter();
        w2i.SetInput(renWin);
        w2i.Modified();
        renWin.Render();
        w2i.Update();
        svtkImageData image = w2i.GetOutput();

        svtkUnsignedCharArray da = (svtkUnsignedCharArray) image.GetPointData().GetScalars();
        byte[] barray = da.GetJavaArray();

        System.out.println("Length of array: " + barray.length);

        svtkUnsignedCharArray nda = new svtkUnsignedCharArray();
        nda.SetJavaArray(barray);

        svtkImageData nimage = new svtkImageData();
        nimage.SetDimensions(image.GetDimensions());
        nimage.SetSpacing(image.GetSpacing());
        nimage.SetOrigin(image.GetOrigin());
        nimage.AllocateScalars(image.GetScalarType(), image.GetNumberOfScalarComponents());
        svtkUnsignedCharArray nida = (svtkUnsignedCharArray) nimage.GetPointData().GetScalars();
        nida.SetJavaArray(barray);

        int retVal0 = svtkJavaTesting.PASSED;

        for (cc = 0; cc <= da.GetMaxId(); cc++) {
          int v1 = 0, v2 = 0, v3 = 0;
          v1 = da.GetValue(cc);
          if (cc <= nda.GetMaxId()) {
            v2 = nda.GetValue(cc);
          } else {
            System.out.println("Cannot find point " + cc + " in nda");
            retVal0 = svtkJavaTesting.FAILED;
          }
          if (cc <= nida.GetMaxId()) {
            v3 = nida.GetValue(cc);
          } else {
            System.out.println("Cannot find point " + cc + " in nida");
            retVal0 = svtkJavaTesting.FAILED;
          }
          if (v1 != v2 || v1 != v3) {
            System.out.println("Wrong point: " + v1 + " <> " + v2 + " <> " + v3);
            retVal0 = svtkJavaTesting.FAILED;
          }
        }

        svtkImageDifference imgDiff = new svtkImageDifference();
        imgDiff.SetInputData(nimage);
        imgDiff.SetImageConnection(w2i.GetOutputPort());
        imgDiff.Update();

        int retVal1 = svtkJavaTesting.PASSED;
        if (imgDiff.GetThresholdedError() != 0) {
          System.out.println("Problem with array conversion. Image difference is: " + imgDiff.GetThresholdedError());
          svtkPNGWriter wr = new svtkPNGWriter();
          wr.SetInputConnection(w2i.GetOutputPort());
          wr.SetFileName("im1.png");
          wr.Write();
          wr.SetInputData(nimage);
          wr.SetFileName("im2.png");
          wr.Write();
          wr.SetInputConnection(imgDiff.GetOutputPort());
          wr.SetFileName("diff.png");
          wr.Write();
          retVal1 = svtkJavaTesting.FAILED;
        }

        int retVal2 = svtkJavaTesting.PASSED;
        if (svtkJavaTesting.IsInteractive()) {
          iren.Start();
        } else {
          retVal2 = svtkJavaTesting.RegressionTest(renWin, 10);
        }

        svtkObject.JAVA_OBJECT_MANAGER.deleteAll();

        if (retVal0 != svtkJavaTesting.PASSED) {
          svtkJavaTesting.Exit(retVal0);
        }
        if (retVal1 != svtkJavaTesting.PASSED) {
          svtkJavaTesting.Exit(retVal1);
        }
        svtkJavaTesting.Exit(retVal2);
      }
    });
  }
}
