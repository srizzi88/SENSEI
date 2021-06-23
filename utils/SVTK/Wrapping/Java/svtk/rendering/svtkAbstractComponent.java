package svtk.rendering;

import java.util.concurrent.locks.ReentrantLock;

import svtk.svtkAxesActor;
import svtk.svtkCamera;
import svtk.svtkGenericRenderWindowInteractor;
import svtk.svtkInteractorStyle;
import svtk.svtkInteractorStyleTrackballCamera;
import svtk.svtkOrientationMarkerWidget;
import svtk.svtkRenderWindow;
import svtk.svtkRenderer;

/**
 * Abstract class that bring most of the SVTK logic to any rendering component
 * regardless its origin. (awt, swt, sing, ...)
 *
 * @param <T>
 *            The concrete type of the graphical component that will contains
 *            the svtkRenderWindow.
 *
 * @authors Sebastien Jourdain - sebastien.jourdain@kitware.com, Kitware Inc 2012
 *          Joachim Pouderoux - joachim.pouderoux@kitware.com, Kitware SAS 2012
 * @copyright This work was supported by CEA/CESTA
 *            Commissariat a l'Energie Atomique et aux Energies Alternatives,
 *            15 avenue des Sablieres, CS 60001, 33116 Le Barp, France.
 */
public abstract class svtkAbstractComponent<T> implements svtkComponent<T> {
  protected svtkRenderWindow renderWindow;
  protected svtkRenderer renderer;
  protected svtkCamera camera;
  protected svtkGenericRenderWindowInteractor windowInteractor;
  protected svtkInteractorForwarder eventForwarder;
  protected ReentrantLock lock;
  protected boolean inRenderCall;

  public svtkAbstractComponent() {
    this(new svtkRenderWindow());
  }

  public svtkAbstractComponent(svtkRenderWindow renderWindowToUse) {
    this.inRenderCall = false;
    this.renderWindow = renderWindowToUse;
    this.renderer = new svtkRenderer();
    this.windowInteractor = new svtkGenericRenderWindowInteractor();
    this.lock = new ReentrantLock();

    // Init interactor
    this.windowInteractor.SetRenderWindow(this.renderWindow);
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

    // Link renderWindow with renderer
    this.renderWindow.AddRenderer(this.renderer);

    // Keep camera around to prevent its creation/deletion in Java world
    this.camera = this.renderer.GetActiveCamera();
  }

  public ReentrantLock getSVTKLock() {
    return this.lock;
  }

  public void resetCamera() {
    if (renderer == null) {
      return; // Nothing to do we are deleted...
    }

    try {
      lock.lockInterruptibly();
      renderer.ResetCamera();
    } catch (InterruptedException e) {
      // Nothing that we can do
    } finally {
      this.lock.unlock();
    }
  }

  public void resetCameraClippingRange() {
    if (renderWindow == null) {
      return; // Nothing to do we are deleted...
    }

    try {
      this.lock.lockInterruptibly();
      renderer.ResetCameraClippingRange();
    } catch (InterruptedException e) {
      // Nothing that we can do
    } finally {
      this.lock.unlock();
    }
  }

  public svtkCamera getActiveCamera() {
    return this.camera;
  }

  public svtkRenderer getRenderer() {
    return this.renderer;
  }

  public svtkRenderWindow getRenderWindow() {
    return this.renderWindow;
  }

  public svtkGenericRenderWindowInteractor getRenderWindowInteractor() {
    return this.windowInteractor;
  }

  public void setInteractorStyle(svtkInteractorStyle style) {
    if (this.windowInteractor != null) {
      this.lock.lock();
      this.windowInteractor.SetInteractorStyle(style);
      this.lock.unlock();
    }
  }

  public void setSize(int w, int h) {
    if (renderWindow == null || windowInteractor == null) {
      return; // Nothing to do we are deleted...
    }

    try {
      lock.lockInterruptibly();
      renderWindow.SetSize(w, h);
      windowInteractor.SetSize(w, h);
    } catch (InterruptedException e) {
      // Nothing that we can do
    } finally {
      this.lock.unlock();
    }
  }

  public void Delete() {
    this.lock.lock();
    this.renderer = null;
    this.camera = null;
    this.windowInteractor = null;
    // removing the renderWindow is let to the superclass
    // because in the very special case of an AWT component
    // under Linux, destroying renderWindow crashes.
    this.lock.unlock();
  }

  public svtkInteractorForwarder getInteractorForwarder() {
    return this.eventForwarder;
  }

  public abstract T getComponent();

  /**
   * Generic helper method used to attach orientation axes to a svtkComponent
   *
   * @param svtkComponent<?>
   */
  public static void attachOrientationAxes(svtkComponent<?> component) {
    // only build this once, because it creates its own renderer.
    // Extra renderers causes issues with resetting.
    svtkAxesActor axes = new svtkAxesActor();
    svtkOrientationMarkerWidget axesWidget = new svtkOrientationMarkerWidget();

    axesWidget.SetOutlineColor(0.9300, 0.5700, 0.1300);
    axesWidget.SetOrientationMarker(axes);
    axesWidget.SetInteractor(component.getRenderWindowInteractor());
    axesWidget.SetDefaultRenderer(component.getRenderer());
    axesWidget.SetViewport(0.0, 0.0, .2, .2);
    axesWidget.EnabledOn();
    axesWidget.InteractiveOff();
  }
}
