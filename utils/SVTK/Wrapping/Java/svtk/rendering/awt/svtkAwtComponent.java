package svtk.rendering.awt;

import java.awt.Canvas;
import java.awt.Dimension;
import java.awt.event.ComponentAdapter;
import java.awt.event.ComponentEvent;

import svtk.svtkObject;
import svtk.svtkRenderWindow;
import svtk.rendering.svtkAbstractComponent;

/**
 * Provide AWT based svtk rendering component
 *
 * @authors Sebastien Jourdain - sebastien.jourdain@kitware.com
 *          Joachim Pouderoux - joachim.pouderoux@kitware.com
 */
public class svtkAwtComponent extends svtkAbstractComponent<Canvas> {
  protected svtkInternalAwtComponent uiComponent;
  protected boolean isWindowCreated;
  protected Runnable onWindowCreatedCallback;

  public svtkAwtComponent() {
    this(new svtkRenderWindow());
  }

  public svtkAwtComponent(svtkRenderWindow renderWindowToUse) {
    super(renderWindowToUse);
    this.isWindowCreated = false;
    this.uiComponent = new svtkInternalAwtComponent(this);
    this.uiComponent.addComponentListener(new ComponentAdapter() {

      public void componentResized(ComponentEvent arg0) {
        Dimension size = svtkAwtComponent.this.uiComponent.getSize();
        svtkAwtComponent.this.setSize(size.width, size.height);
      }
    });
  }

  public void Render() {
    // Make sure we can render
    if (inRenderCall || renderer == null || renderWindow == null) {
      return;
    }

    // Try to render
    try {
      lock.lockInterruptibly();
      inRenderCall = true;

      // Initialize the window only once
      if (!isWindowCreated) {
        uiComponent.RenderCreate(renderWindow);
        setSize(uiComponent.getWidth(), uiComponent.getHeight());
        isWindowCreated = true;
      }

      // Trigger the real render
      renderWindow.Render();

      // Execute callback if need be
      if(this.onWindowCreatedCallback != null) {
        this.onWindowCreatedCallback.run();
        this.onWindowCreatedCallback = null;
      }
    } catch (InterruptedException e) {
      // Nothing that we can do except skipping execution
    } finally {
      lock.unlock();
      inRenderCall = false;
    }
  }

  public Canvas getComponent() {
    return this.uiComponent;
  }

  public void Delete() {
    this.lock.lock();

    // We prevent any further rendering
    inRenderCall = true;

    if (this.uiComponent.getParent() != null) {
      this.uiComponent.getParent().remove(this.uiComponent);
    }
    super.Delete();

    // On linux we prefer to have a memory leak instead of a crash
    if (!this.renderWindow.GetClassName().equals("svtkXOpenGLRenderWindow")) {
      this.renderWindow = null;
    } else {
      System.out.println("The renderwindow has been kept around to prevent a crash");
    }
    this.lock.unlock();
    svtkObject.JAVA_OBJECT_MANAGER.gc(false);
  }

  /**
   * @return true if the graphical component has been properly set and
   *         operation can be performed on it.
   */
  public boolean isWindowSet() {
    return this.isWindowCreated;
  }

  /**
   * Set a callback that gets called once the window is properly created and can be
   * customized in its settings.
   *
   * Once called the callback will be released.
   *
   * @param callback
   */
  public void setWindowReadyCallback(Runnable callback) {
    this.onWindowCreatedCallback = callback;
  }

  /**
   * Just allow class in same package to affect inRenderCall boolean
   *
   * @param value
   */
  protected void updateInRenderCall(boolean value) {
    this.inRenderCall = value;
  }
}
