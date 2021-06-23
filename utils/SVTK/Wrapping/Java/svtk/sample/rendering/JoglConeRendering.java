package svtk.sample.rendering;

import java.awt.BorderLayout;
import java.awt.event.KeyEvent;
import java.awt.event.KeyListener;
import java.awt.event.MouseEvent;
import java.util.Arrays;

import javax.swing.JFrame;
import javax.swing.SwingUtilities;

import svtk.svtkActor;
import svtk.svtkBoxRepresentation;
import svtk.svtkBoxWidget2;
import svtk.svtkCell;
import svtk.svtkCellPicker;
import svtk.svtkConeSource;
import svtk.svtkLookupTable;
import svtk.svtkNativeLibrary;
import svtk.svtkPolyDataMapper;
import svtk.svtkScalarBarRepresentation;
import svtk.svtkScalarBarWidget;
import svtk.svtkTransform;
import svtk.rendering.svtkAbstractEventInterceptor;
import svtk.rendering.svtkEventInterceptor;
import svtk.rendering.jogl.svtkAbstractJoglComponent;
import svtk.rendering.jogl.svtkJoglCanvasComponent;
import svtk.rendering.jogl.svtkJoglPanelComponent;

public class JoglConeRendering {
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
    final boolean usePanel = Boolean.getBoolean("usePanel");

    SwingUtilities.invokeLater(new Runnable() {
      public void run() {
        // build SVTK Pipeline
        svtkConeSource cone = new svtkConeSource();
        cone.SetResolution(8);
        cone.Update();

        svtkPolyDataMapper coneMapper = new svtkPolyDataMapper();
        coneMapper.SetInputConnection(cone.GetOutputPort());

        final svtkActor coneActor = new svtkActor();
        coneActor.SetMapper(coneMapper);

        // SVTK rendering part
        final svtkAbstractJoglComponent<?> joglWidget = usePanel ? new svtkJoglPanelComponent() : new svtkJoglCanvasComponent();
        System.out.println("We are using " + joglWidget.getComponent().getClass().getName() + " for the rendering.");

        joglWidget.getRenderer().AddActor(coneActor);

        // Add orientation axes
        svtkAbstractJoglComponent.attachOrientationAxes(joglWidget);

        // Add Scalar bar widget
        svtkLookupTable lut = new svtkLookupTable();
        lut.SetHueRange(.66, 0);
        lut.Build();
        svtkScalarBarWidget scalarBar = new svtkScalarBarWidget();
        scalarBar.SetInteractor(joglWidget.getRenderWindowInteractor());

        scalarBar.GetScalarBarActor().SetTitle("Example");
        scalarBar.GetScalarBarActor().SetLookupTable(lut);
        scalarBar.GetScalarBarActor().SetOrientationToHorizontal();
        scalarBar.GetScalarBarActor().SetTextPositionToPrecedeScalarBar();
        svtkScalarBarRepresentation srep = (svtkScalarBarRepresentation) scalarBar.GetRepresentation();
        srep.SetPosition(0.5, 0.053796);
        srep.SetPosition2(0.33, 0.106455);
        //scalarBar.ProcessEventsOff();
        scalarBar.EnabledOn();
        scalarBar.RepositionableOn();

        // Add interactive 3D Widget
        final svtkBoxRepresentation representation = new svtkBoxRepresentation();
        representation.SetPlaceFactor(1.25);
        representation.PlaceWidget(cone.GetOutput().GetBounds());

        final svtkBoxWidget2 boxWidget = new svtkBoxWidget2();
        boxWidget.SetRepresentation(representation);
        boxWidget.SetInteractor(joglWidget.getRenderWindowInteractor());
        boxWidget.SetPriority(1);

        final Runnable callback = new Runnable() {
          svtkTransform trasform = new svtkTransform();

          public void run() {
            svtkBoxRepresentation rep = (svtkBoxRepresentation) boxWidget.GetRepresentation();
            rep.GetTransform(trasform);
            coneActor.SetUserTransform(trasform);
          }
        };

        // Bind widget
        boxWidget.AddObserver("InteractionEvent", callback, "run");
        representation.VisibilityOn();
        representation.HandlesOn();
        boxWidget.SetEnabled(1);
        boxWidget.SetMoveFacesEnabled(1);

        // Add cell picker
        final svtkCellPicker picker = new svtkCellPicker();
        Runnable pickerCallback = new Runnable() {
          public void run() {
            if(picker.GetCellId() != -1) {
              svtkCell cell = picker.GetDataSet().GetCell(picker.GetCellId());
              System.out.println("Pick cell: " +  picker.GetCellId() + " - Bounds: " + Arrays.toString(cell.GetBounds()));
            }
          }
        };
        joglWidget.getRenderWindowInteractor().SetPicker(picker);
        picker.AddObserver("EndPickEvent", pickerCallback, "run");

        // Bind pick action to double-click
        joglWidget.getInteractorForwarder().setEventInterceptor(new svtkAbstractEventInterceptor() {

          public boolean mouseClicked(MouseEvent e) {
            // Request picking action on double-click
            final double[] position = {e.getX(), joglWidget.getComponent().getHeight() - e.getY(), 0};
            if(e.getClickCount() == 2) {
              System.out.println("Click trigger the picking (" + position[0] + ", " +position[1] + ")");
              picker.Pick(position, joglWidget.getRenderer());
            }

            // We let the InteractionStyle process the event anyway
            return false;
          }
        });

        // UI part
        JFrame frame = new JFrame("SimpleSVTK");
        frame.setDefaultCloseOperation(JFrame.EXIT_ON_CLOSE);
        frame.getContentPane().setLayout(new BorderLayout());
        frame.getContentPane().add(joglWidget.getComponent(),
            BorderLayout.CENTER);
        frame.setSize(400, 400);
        frame.setLocationRelativeTo(null);
        frame.setVisible(true);
        joglWidget.resetCamera();
        joglWidget.getComponent().requestFocus();

        // Add r:ResetCamera and q:Quit key binding
        joglWidget.getComponent().addKeyListener(new KeyListener() {
          @Override
          public void keyTyped(KeyEvent e) {
            if (e.getKeyChar() == 'r') {
              joglWidget.resetCamera();
            } else if (e.getKeyChar() == 'q') {
              System.exit(0);
            }
          }

          @Override
          public void keyReleased(KeyEvent e) {
          }

          @Override
          public void keyPressed(KeyEvent e) {
          }
        });
      }
    });
  }
}
