package svtk.test;

import svtk.svtkDoubleArray;
import svtk.svtkJavaTesting;
import svtk.svtkObject;
import svtk.svtkQuadric;
import svtk.svtkSampleFunction;

/**
 * This test should go on forever without crashing.
 */
public class JavaDelete {

  public static void main(String[] args) {
    try {
      svtkJavaTesting.Initialize(args, true);

      // Start working code
      long timeout = System.currentTimeMillis() + 60000; // +1 minute
      while (System.currentTimeMillis() < timeout) {
        svtkDoubleArray arr = new svtkDoubleArray();
        arr.Delete();

        svtkQuadric quadric = new svtkQuadric();
        svtkSampleFunction sample = new svtkSampleFunction();
        sample.SetSampleDimensions(30, 30, 30);
        sample.SetImplicitFunction(quadric);
        sample.GetImplicitFunction();
        sample.Delete();
        quadric.Delete();

        // Make sure the Java SVTK object map is empty
        if (svtkObject.JAVA_OBJECT_MANAGER.getSize() > 1) { // svtkTesting
          System.out.println(svtkObject.JAVA_OBJECT_MANAGER.gc(true).listKeptReferenceToString());
          throw new RuntimeException("There shouldn't have any SVTK object inside the map as we are using Delete().");
        }
      }

      svtkJavaTesting.Exit(svtkJavaTesting.PASSED);
    } catch (Throwable e) {
      e.printStackTrace();
      svtkJavaTesting.Exit(svtkJavaTesting.FAILED);
    }
  }
}
