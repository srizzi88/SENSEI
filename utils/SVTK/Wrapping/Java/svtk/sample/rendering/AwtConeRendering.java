package svtk.sample.rendering;

import java.awt.BorderLayout;

import javax.swing.JFrame;
import javax.swing.SwingUtilities;

import svtk.svtkActor;
import svtk.svtkConeSource;
import svtk.svtkNativeLibrary;
import svtk.svtkPolyDataMapper;
import svtk.rendering.awt.svtkAwtComponent;

public class AwtConeRendering {
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
    SwingUtilities.invokeLater(new Runnable() {
      public void run() {
        // build SVTK Pipeline
        svtkConeSource cone = new svtkConeSource();
        cone.SetResolution(8);

        svtkPolyDataMapper coneMapper = new svtkPolyDataMapper();
        coneMapper.SetInputConnection(cone.GetOutputPort());

        svtkActor coneActor = new svtkActor();
        coneActor.SetMapper(coneMapper);

        // SVTK rendering part
        svtkAwtComponent awtWidget = new svtkAwtComponent();
        awtWidget.getRenderer().AddActor(coneActor);

        // UI part
        JFrame frame = new JFrame("SimpleSVTK");
        frame.setDefaultCloseOperation(JFrame.EXIT_ON_CLOSE);
        frame.getContentPane().setLayout(new BorderLayout());
        frame.getContentPane().add(awtWidget.getComponent(),
            BorderLayout.CENTER);
        frame.setSize(400, 400);
        frame.setLocationRelativeTo(null);
        frame.setVisible(true);
      }
    });
  }
}
