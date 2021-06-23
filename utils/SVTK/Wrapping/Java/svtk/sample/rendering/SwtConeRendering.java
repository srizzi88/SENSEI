package svtk.sample.rendering;

import org.eclipse.swt.events.ControlEvent;
import org.eclipse.swt.events.ControlListener;
import org.eclipse.swt.widgets.Display;
import org.eclipse.swt.widgets.Shell;

import svtk.svtkActor;
import svtk.svtkConeSource;
import svtk.svtkNativeLibrary;
import svtk.svtkPolyDataMapper;
import svtk.rendering.swt.svtkSwtComponent;

public class SwtConeRendering {
  // -----------------------------------------------------------------
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

  public static void main(String[] args) {

    Display display = new Display();
    Shell shell = new Shell(display);

    // build SVTK Pipeline
    svtkConeSource cone = new svtkConeSource();
    cone.SetResolution(8);

    svtkPolyDataMapper coneMapper = new svtkPolyDataMapper();
    coneMapper.SetInputConnection(cone.GetOutputPort());

    svtkActor coneActor = new svtkActor();
    coneActor.SetMapper(coneMapper);

    // SVTK rendering part
    final svtkSwtComponent swtWidget = new svtkSwtComponent(shell);
    swtWidget.getRenderer().AddActor(coneActor);

    shell.addControlListener(new ControlListener() {

      @Override
      public void controlResized(ControlEvent e) {
        if (e.widget instanceof Shell) {
          Shell s = (Shell) e.widget;
          swtWidget.setSize(s.getClientArea().width, s.getClientArea().height);
        }
      }

      @Override
      public void controlMoved(ControlEvent e) {
        // TODO Auto-generated method stub

      }
    });

    shell.pack();
    shell.open();
    while (!shell.isDisposed()) {
      if (!display.readAndDispatch())
        display.sleep();
    }
    display.dispose();
  }
}
