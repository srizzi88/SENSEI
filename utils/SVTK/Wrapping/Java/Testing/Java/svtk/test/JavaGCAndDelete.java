package svtk.test;

import svtk.svtkActor;
import svtk.svtkArrowSource;
import svtk.svtkJavaTesting;
import svtk.svtkObject;
import svtk.svtkPolyDataMapper;

/**
 * This test should run forever without running out of memory or
 * crashing. It should work with or without the Delete() calls present.
 */
public class JavaGCAndDelete {

  public static void main(final String[] args) {
    svtkJavaTesting.Initialize(args);
    long timeout = System.currentTimeMillis() + 60000; // +1 minute
    int i = 0;
    int k = 0;
    while (System.currentTimeMillis() < timeout) {
      final svtkArrowSource arrowSource = new svtkArrowSource();
      final svtkPolyDataMapper mapper = new svtkPolyDataMapper();
      mapper.SetInputConnection(arrowSource.GetOutputPort());
      final svtkActor actor = new svtkActor();
      actor.SetMapper(mapper);

      arrowSource.GetOutput().Delete();
      arrowSource.Delete();
      mapper.Delete();
      actor.Delete();

      ++i;
      if (i >= 10000) {
        ++k;
        System.out.println(svtkObject.JAVA_OBJECT_MANAGER.gc(true).listKeptReferenceToString());
        System.out.println(k * 10000);
        i = 0;
      }
    }
    svtkJavaTesting.Exit(svtkJavaTesting.PASSED);
  }

}
