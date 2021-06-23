package svtk;

import java.io.File;
import java.util.concurrent.Executors;
import java.util.concurrent.ScheduledExecutorService;
import java.util.concurrent.TimeUnit;

import svtk.svtkObject;
import svtk.svtkRenderWindow;
import svtk.svtkSettings;
import svtk.svtkTesting;

public class svtkJavaTesting {
  public static final int FAILED = 0;
  public static final int PASSED = 1;
  public static final int NOT_RUN = 2;
  public static final int DO_INTERACTOR = 3;

  private static int LoadLib(String lib, boolean verbose) {
    try {
      if (verbose) {
        System.out.println("Try to load: " + lib);
      }

      if(!new File(lib).exists()) {
        if(verbose) {
          System.out.println("File does not exist: " + lib);
          return 0;
        }
      }

      Runtime.getRuntime().load(lib);
    } catch (UnsatisfiedLinkError e) {
      if (verbose) {
        System.out.println("Failed to load: " + lib);
        e.printStackTrace();
      }
      return 0;
    }
    if (verbose) {
      System.out.println("Successfully loaded: " + lib);
    }
    return 1;
  }

  private static void LoadLibrary(String path, String library, boolean verbose) {
    String lname = System.mapLibraryName(library);
    String sep = System.getProperty("file.separator");
    String libname = path + sep + lname;
    String releaselibname = path + sep + "Release" + sep + lname;
    String debuglibname = path + sep + "Debug" + sep + lname;
    if (svtkJavaTesting.LoadLib(library, verbose) != 1 //
        && svtkJavaTesting.LoadLib(libname, verbose) != 1
        && svtkJavaTesting.LoadLib(releaselibname, verbose) != 1
        && svtkJavaTesting.LoadLib(debuglibname, verbose) != 1) {
      System.out.println("Problem loading appropriate library");
    }
  }

  public static void Initialize(String[] args) {
    svtkJavaTesting.Initialize(args, false);
  }

  public static void Initialize(String[] args, boolean verbose) {
    String lpath = svtkSettings.GetSVTKLibraryDir();
    if (lpath != null) {
      String path_separator = System.getProperty("path.separator");
      String s = System.getProperty("java.library.path");
      s = s + path_separator + lpath;
      System.setProperty("java.library.path", s);
    }
    // String lname = System.mapLibraryName("svtkCommonJava");
    String[] kits = svtkSettings.GetKits();
    int cc;
    for (cc = 0; cc < kits.length; cc++) {
      svtkJavaTesting.LoadLibrary(lpath, kits[cc] + "Java", verbose);
    }
    svtkJavaTesting.Tester = new svtk.svtkTesting();
    for (cc = 0; cc < args.length; cc++) {
      svtkJavaTesting.Tester.AddArgument(args[cc]);
    }
  }

  public static boolean IsInteractive() {
    if (svtkJavaTesting.Tester.IsInteractiveModeSpecified() == 0) {
      return false;
    }
    return true;
  }

  public static void Exit(int retVal) {
    svtkJavaTesting.Tester = null;
    System.gc();
    svtkObjectBase.JAVA_OBJECT_MANAGER.gc(true);

    if (retVal == svtkJavaTesting.FAILED || retVal == svtkJavaTesting.NOT_RUN) {
      System.out.println("Test failed or was not run");
      System.exit(1);
    }
    System.out.println("Test passed");
    System.exit(0);
  }

  public static int RegressionTest(svtkRenderWindow renWin, int threshold) {
    svtkJavaTesting.Tester.SetRenderWindow(renWin);

    if (svtkJavaTesting.Tester.RegressionTest(threshold) == svtkJavaTesting.PASSED) {
      return svtkJavaTesting.PASSED;
    }
    System.out.println("Image difference: " + svtkJavaTesting.Tester.GetImageDifference());
    return svtkJavaTesting.FAILED;
  }

  public static void StartTimeoutExit(long time, TimeUnit unit) {
    ScheduledExecutorService killerThread = Executors.newSingleThreadScheduledExecutor();
    Runnable killer = new Runnable() {
      public void run() {
        System.exit(0);
      }
    };
    killerThread.schedule(killer, time, unit);
  }

  public static svtkJavaGarbageCollector StartGCInEDT(long time, TimeUnit unit) {
    svtkJavaGarbageCollector gc = svtkObjectBase.JAVA_OBJECT_MANAGER.getAutoGarbageCollector();
    gc.SetScheduleTime(time, unit);
    gc.SetAutoGarbageCollection(true);
    return gc;
  }

  private static svtkTesting Tester = null;
}
