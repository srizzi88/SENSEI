package svtk.test;

import java.lang.reflect.InvocationTargetException;
import java.util.concurrent.TimeUnit;

import svtk.svtkJavaGarbageCollector;
import svtk.svtkJavaTesting;
import svtk.svtkPoints;
import svtk.svtkUnstructuredGrid;

/**
 * This test run concurrently thread that create Java view of SVTK objects and
 * the EDT that collect those objects as well as another thread.
 *
 * Based on the changes done to prevent concurrency during creation/deletion of
 * SVTK object this application shouldn't crash.
 *
 * @author sebastien jourdain - sebastien.jourdain@kitware.com
 */
public class ConcurrencyGC {

  public static void main(String[] args) throws InterruptedException, InvocationTargetException {
    try {
      svtkJavaTesting.Initialize(args, true);

      // Setup working runnable
      Runnable workingJob = new Runnable() {
        public void run() {
          try {
            svtkUnstructuredGrid grid = new svtkUnstructuredGrid();
            grid.SetPoints(new svtkPoints());
            svtkPoints p;
            long timeout = System.currentTimeMillis() + 60000; // +1 minute
            while (System.currentTimeMillis() < timeout) {
              p = grid.GetPoints();
              if (p == null) {
                throw new RuntimeException("Invalid pointer null");
              }
              if (p.GetReferenceCount() != 2) {
                throw new RuntimeException("Invalid reference count of " + p.GetReferenceCount());
              }
            }
          } catch (Throwable e) {
            e.printStackTrace();
            svtkJavaTesting.Exit(svtkJavaTesting.FAILED);
          }
        }
      };

      // Start threads for concurrency (2xwork + 1xGC + 1xGCinEDT)
      Thread worker1 = new Thread(workingJob);
      Thread worker2 = new Thread(workingJob);

      // Start working thread
      worker1.start();
      worker2.start();

      // Setup GC
      svtkJavaGarbageCollector gc = svtkJavaTesting.StartGCInEDT(10, TimeUnit.MILLISECONDS); // Start periodic GC in EDT
      new Thread(gc.GetDeleteRunnable()).start();                                          // Start GC in tight loop

      // If worker finished close everything
      worker1.join();
      worker2.join();
      gc.Stop();
      svtkJavaTesting.Exit(svtkJavaTesting.PASSED);

    } catch (Throwable e) {
      e.printStackTrace();
      svtkJavaTesting.Exit(svtkJavaTesting.FAILED);
    }
  }
}
