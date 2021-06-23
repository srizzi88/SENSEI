package svtk.sample;

import java.awt.BorderLayout;
import java.awt.GridLayout;

import javax.swing.JFrame;
import javax.swing.JPanel;

import svtk.AxesActor;
import svtk.svtkActor;
import svtk.svtkCanvas;
import svtk.svtkConeSource;
import svtk.svtkNativeLibrary;
import svtk.svtkObject;
import svtk.svtkPolyDataMapper;
import svtk.svtkReferenceInformation;

public class SVTKCanvas extends JPanel {
  private static final long serialVersionUID = 1L;

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

  // -----------------------------------------------------------------

  public SVTKCanvas() {
    setLayout(new BorderLayout());
    // Create the buttons.
    svtkCanvas renWin = new svtkCanvas();
    add(renWin, BorderLayout.CENTER);
    svtkConeSource cone = new svtkConeSource();
    cone.SetResolution(8);
    svtkPolyDataMapper coneMapper = new svtkPolyDataMapper();
    coneMapper.SetInputConnection(cone.GetOutputPort());

    svtkActor coneActor = new svtkActor();
    coneActor.SetMapper(coneMapper);

    renWin.GetRenderer().AddActor(coneActor);
    AxesActor aa = new AxesActor(renWin.GetRenderer());
    renWin.GetRenderer().AddActor(aa);
  }

  public static void main(String s[]) {
    SVTKCanvas panel = new SVTKCanvas();
    SVTKCanvas panel2 = new SVTKCanvas();

    JFrame frame = new JFrame("SVTK Canvas Test");
    frame.getContentPane().setLayout(new GridLayout(2, 1));
    frame.setDefaultCloseOperation(JFrame.EXIT_ON_CLOSE);
    frame.getContentPane().add(panel);
    frame.getContentPane().add(panel2);
    frame.setSize(600, 600);
    frame.setLocationRelativeTo(null);
    frame.setVisible(true);

    svtkReferenceInformation infos = svtkObject.JAVA_OBJECT_MANAGER.gc(true);
    System.out.println(infos);
    System.out.println(infos.listRemovedReferenceToString());
    System.out.println(infos.listKeptReferenceToString());
  }
}
