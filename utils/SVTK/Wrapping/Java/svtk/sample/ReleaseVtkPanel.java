package svtk.sample;

import java.awt.BorderLayout;
import java.awt.GridLayout;
import java.awt.event.ActionEvent;
import java.awt.event.ActionListener;
import java.awt.event.WindowAdapter;
import java.awt.event.WindowEvent;
import java.util.concurrent.TimeUnit;

import javax.swing.JButton;
import javax.swing.JComponent;
import javax.swing.JFrame;
import javax.swing.JPanel;
import javax.swing.SwingUtilities;

import svtk.svtkActor;
import svtk.svtkConeSource;
import svtk.svtkNativeLibrary;
import svtk.svtkObject;
import svtk.svtkPanel;
import svtk.svtkPolyDataMapper;

/**
 * This test create in a secondary frame a SVTK application. When that frame get
 * closed we want to make sure all the SVTK memory is released without any crash.
 *
 * @author sebastien jourdain - sebastien.jourdain@kitware.com
 */
public class ReleaseVtkPanel {

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

  private static class VtkApplication extends JPanel {
    private static final long serialVersionUID = -6486953735097088917L;
    private svtkPanel panel3dA;
    private svtkPanel panel3dB;

    public VtkApplication() {
      super(new GridLayout(1, 2));
      panel3dA = new svtkPanel();
      panel3dB = new svtkPanel();

      svtkConeSource cone = new svtkConeSource();
      svtkPolyDataMapper mapper = new svtkPolyDataMapper();
      svtkActor actor = new svtkActor();

      mapper.SetInputConnection(cone.GetOutputPort());
      actor.SetMapper(mapper);

      panel3dA.GetRenderer().AddActor(actor);
      add(panel3dA);

      panel3dB.GetRenderer().AddActor(actor);
      add(panel3dB);
    }

    public void Delete() {
      panel3dA.Delete();
      panel3dB.Delete();
    }

  }

  public static void main(String[] args) {
    SwingUtilities.invokeLater(new Runnable() {

      public void run() {
        // Setup GC to run every 1 second in EDT
        svtkObject.JAVA_OBJECT_MANAGER.getAutoGarbageCollector().SetScheduleTime(5, TimeUnit.SECONDS);
        svtkObject.JAVA_OBJECT_MANAGER.getAutoGarbageCollector().SetDebug(true);
        svtkObject.JAVA_OBJECT_MANAGER.getAutoGarbageCollector().Start();

        JButton startSVTKApp = new JButton("Start SVTK application");
        startSVTKApp.addActionListener(new ActionListener() {

          public void actionPerformed(ActionEvent e) {
            final VtkApplication app = new VtkApplication();
            JFrame f = buildFrame("VtkApp", app, 400, 200);
            f.addWindowListener(new WindowAdapter() {
              public void windowClosing(WindowEvent e) {
                app.Delete();
              }
            });
            f.setVisible(true);
          }
        });
        JFrame mainFrame = buildFrame("Launcher", startSVTKApp, 200, 200);
        mainFrame.setDefaultCloseOperation(JFrame.EXIT_ON_CLOSE);
        mainFrame.setVisible(true);
      }
    });
  }

  public static JFrame buildFrame(String title, JComponent content, int width, int height) {
    JFrame f = new JFrame(title);
    f.getContentPane().setLayout(new BorderLayout());
    f.getContentPane().add(content, BorderLayout.CENTER);
    f.setDefaultCloseOperation(JFrame.DISPOSE_ON_CLOSE);
    f.setSize(width, height);
    f.setLocationRelativeTo(null);
    return f;
  }
}
