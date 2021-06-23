import svtk.*;

public class HelloWorld {

  // Load SVTK library and print which library was not properly loaded
  static {
    if (!svtkNativeLibrary.LoadAllNativeLibraries()) {
      for (svtkNativeLibrary lib : svtkNativeLibrary.values()) {
        if (!lib.IsLoaded()) {
          System.out.println(lib.GetLibraryName() + " not loaded");
        }
      }
    }
    svtkNativeLibrary.DisableOutputWindow(null);
  }

  public static void main(String args[]) {
    svtkRandomGraphSource source = new svtkRandomGraphSource();

    svtkGraphLayoutView view = new svtkGraphLayoutView();
    view.AddRepresentationFromInputConnection(source.GetOutputPort());

    view.ResetCamera();
    view.Render();
    view.GetInteractor().Start();
  }
}
