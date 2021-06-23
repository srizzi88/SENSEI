package svtk.test;

import svtk.svtkIdTypeArray;
import svtk.svtkJavaTesting;
import svtk.svtkObject;
import svtk.svtkObjectBase;
import svtk.svtkReferenceInformation;
import svtk.svtkSelection;
import svtk.svtkSelectionNode;

/**
 * This test should run indefinitely, occasionally outputting
 * "N references deleted".
 *
 * It is an example of how to execute a non-interactive intense processing Java
 * script using SVTK with manual garbage collection.
 */
public class ManualGC {

  private static svtkIdTypeArray createSelection() {
    svtkSelection sel = new svtkSelection();
    svtkSelectionNode node = new svtkSelectionNode();
    svtkIdTypeArray arr = new svtkIdTypeArray();
    node.SetSelectionList(arr);
    sel.AddNode(node);
    return arr;
  }

  public static void main(String[] args) {
    try {
      svtkJavaTesting.Initialize(args, true);
      int count = 0;
      long timeout = System.currentTimeMillis() + 60000; // +1 minute
      while (System.currentTimeMillis() < timeout) {
        // When the selection is deleted,
        // it will decrement the array's reference count.
        // If GC is done on a different thread, this will
        // interfere with the Register/Delete calls on
        // this thread and cause a crash. In general, the code
        // executed in a C++ destructor can do anything, so it
        // is never safe to delete objects on one thread while
        // using them on another.
        //
        // Thus we no longer implement finalize() for SVTK objects.
        // We must manually call
        // svtkObject.JAVA_OBJECT_MANAGER.gc(true/false) when we
        // want to collect unused SVTK objects.
        svtkIdTypeArray arr = createSelection();
        for (int i = 0; i < 10000; ++i) {
          arr.Register(null);
          svtkObjectBase.SVTKDeleteReference(arr.GetSVTKId());
        }
        ++count;
        if (count % 100 == 0) {
          svtkReferenceInformation infos = svtkObject.JAVA_OBJECT_MANAGER.gc(false);
          System.out.println(infos.toString());
        }
      }
      svtkJavaTesting.Exit(svtkJavaTesting.PASSED);
    } catch (Throwable e) {
      e.printStackTrace();
      svtkJavaTesting.Exit(svtkJavaTesting.FAILED);
    }
  }
}
