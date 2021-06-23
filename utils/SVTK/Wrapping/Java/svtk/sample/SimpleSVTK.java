package svtk.sample;

import java.awt.BorderLayout;
import java.awt.event.ActionEvent;
import java.awt.event.ActionListener;

import javax.swing.JButton;
import javax.swing.JFrame;
import javax.swing.JPanel;
import javax.swing.SwingUtilities;

import svtk.svtkActor;
import svtk.svtkConeSource;
import svtk.svtkNativeLibrary;
import svtk.svtkPanel;
import svtk.svtkPolyDataMapper;

/**
 * An application that displays a 3D cone. The button allow to close the
 * application
 */
public class SimpleSVTK extends JPanel implements ActionListener {
  private static final long serialVersionUID = 1L;
  private svtkPanel renWin;
  private JButton exitButton;

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
  public SimpleSVTK() {
    super(new BorderLayout());

    // build SVTK Pipeline
    svtkConeSource cone = new svtkConeSource();
    cone.SetResolution(8);

    svtkPolyDataMapper coneMapper = new svtkPolyDataMapper();
    coneMapper.SetInputConnection(cone.GetOutputPort());

    svtkActor coneActor = new svtkActor();
    coneActor.SetMapper(coneMapper);

    renWin = new svtkPanel();
    renWin.GetRenderer().AddActor(coneActor);

    // Add Java UI components
    exitButton = new JButton("Exit");
    exitButton.addActionListener(this);

    add(renWin, BorderLayout.CENTER);
    add(exitButton, BorderLayout.SOUTH);
  }

  /** An ActionListener that listens to the button. */
  public void actionPerformed(ActionEvent e) {
    if (e.getSource().equals(exitButton)) {
      System.exit(0);
    }
  }

  public static void main(String s[]) {
    SwingUtilities.invokeLater(new Runnable() {
      public void run() {
        JFrame frame = new JFrame("SimpleSVTK");
        frame.setDefaultCloseOperation(JFrame.EXIT_ON_CLOSE);
        frame.getContentPane().setLayout(new BorderLayout());
        frame.getContentPane().add(new SimpleSVTK(), BorderLayout.CENTER);
        frame.setSize(400, 400);
        frame.setLocationRelativeTo(null);
        frame.setVisible(true);
      }
    });
  }
}
