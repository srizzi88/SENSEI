package svtk.rendering.awt;

import java.awt.event.MouseEvent;
import java.awt.event.MouseMotionAdapter;
import java.util.concurrent.locks.ReentrantLock;

import svtk.svtkCamera;
import svtk.svtkGenericRenderWindowInteractor;
import svtk.svtkInteractorStyle;
import svtk.svtkInteractorStyleTrackballCamera;
import svtk.svtkPanel;
import svtk.svtkRenderWindow;
import svtk.svtkRenderer;
import svtk.rendering.svtkComponent;
import svtk.rendering.svtkInteractorForwarder;

/**
 * Provide AWT based svtk rendering component using the svtkPanel class
 * while exposing everything as a new rendering component.
 *
 * @author Sebastien Jourdain - sebastien.jourdain@kitware.com
 *
 * Notes: This class should be replaced down the road by the svtkAwtComponent
 *        but on some platform such as Windows the svtkAwtComponent
 *        produce a runtime error regarding invalid pixel format while
 *        the svtkPanelComponent which use svtkPanel works fine.
 *        For now, this class provide a good substitute with just a minor overhead.
 */

public class svtkPanelComponent implements svtkComponent<svtkPanel> {
  protected svtkPanel panel;
  protected ReentrantLock lock;
  protected svtkGenericRenderWindowInteractor windowInteractor;
  protected svtkInteractorForwarder eventForwarder;

  public svtkPanelComponent() {
    this.panel = new svtkPanel();
    this.lock = new ReentrantLock();

    // Init interactor
    this.windowInteractor = new svtkGenericRenderWindowInteractor();
    this.windowInteractor.SetRenderWindow(this.panel.GetRenderWindow());
    this.windowInteractor.TimerEventResetsTimerOff();

    this.windowInteractor.SetSize(200, 200);
    this.windowInteractor.ConfigureEvent();

    // Update style
    svtkInteractorStyleTrackballCamera style = new svtkInteractorStyleTrackballCamera();
    this.windowInteractor.SetInteractorStyle(style);

    // Setup event forwarder
    this.eventForwarder = new svtkInteractorForwarder(this);
    this.windowInteractor.AddObserver("CreateTimerEvent", this.eventForwarder, "StartTimer");
    this.windowInteractor.AddObserver("DestroyTimerEvent", this.eventForwarder, "DestroyTimer");

    // Remove unwanted listeners
    this.panel.removeKeyListener(this.panel);
    this.panel.removeMouseListener(this.panel);
    this.panel.removeMouseMotionListener(this.panel);
    this.panel.removeMouseWheelListener(this.panel);

    // Add mouse listener that update interactor
    this.panel.addMouseListener(this.eventForwarder);
    this.panel.addMouseMotionListener(this.eventForwarder);
    this.panel.addMouseWheelListener(this.eventForwarder);

    // Make sure we update the light position when interacting
    this.panel.addMouseMotionListener(new MouseMotionAdapter() {
      public void mouseDragged(MouseEvent e) {
        panel.UpdateLight();
      }
    });
  }

  public void resetCamera() {
    this.panel.resetCamera();
  }

  public void resetCameraClippingRange() {
    this.panel.resetCameraClippingRange();
  }

  public svtkCamera getActiveCamera() {
    return this.panel.GetRenderer().GetActiveCamera();
  }

  public svtkRenderer getRenderer() {
    return this.panel.GetRenderer();
  }

  public svtkRenderWindow getRenderWindow() {
    return this.panel.GetRenderWindow();
  }

  public svtkGenericRenderWindowInteractor getRenderWindowInteractor() {
    return this.windowInteractor;
  }

  public void setInteractorStyle(svtkInteractorStyle style) {
    this.getRenderWindowInteractor().SetInteractorStyle(style);
  }

  public void setSize(int w, int h) {
    this.panel.setSize(w, h);
    this.getRenderWindowInteractor().SetSize(w, h);
  }

  public svtkPanel getComponent() {
    return this.panel;
  }

  public void Delete() {
    this.panel.Delete();
  }

  public void Render() {
    this.panel.Render();
  }

  public svtkInteractorForwarder getInteractorForwarder() {
    return this.eventForwarder;
  }

  public ReentrantLock getSVTKLock() {
    return this.lock;
  }
}
