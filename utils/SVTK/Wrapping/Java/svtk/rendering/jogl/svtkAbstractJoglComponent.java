package svtk.rendering.jogl;

import com.jogamp.opengl.GLAutoDrawable;
import com.jogamp.opengl.GLContext;
import com.jogamp.opengl.GLEventListener;

import svtk.svtkGenericOpenGLRenderWindow;
import svtk.svtkObject;
import svtk.svtkRenderWindow;
import svtk.rendering.svtkAbstractComponent;
import svtk.rendering.svtkInteractorForwarder;

/**
 * Provide JOGL based rendering component for SVTK
 *
 * @author Sebastien Jourdain - sebastien.jourdain@kitware.com
 */
public class svtkAbstractJoglComponent<T extends java.awt.Component> extends svtkAbstractComponent<T> {

  protected T uiComponent;
  protected boolean isWindowCreated;
  protected GLEventListener glEventListener;
  protected svtkGenericOpenGLRenderWindow glRenderWindow;


  public svtkAbstractJoglComponent(svtkRenderWindow renderWindowToUse, T glContainer) {
    super(renderWindowToUse);
    this.isWindowCreated = false;
    this.uiComponent = glContainer;
    this.glRenderWindow = (svtkGenericOpenGLRenderWindow) renderWindowToUse;
    this.glRenderWindow.SetIsDirect(1);
    this.glRenderWindow.SetSupportsOpenGL(1);
    this.glRenderWindow.SetIsCurrent(true);

    // Create the JOGL Event Listener
    this.glEventListener = new GLEventListener() {
      public void init(GLAutoDrawable drawable) {
        svtkAbstractJoglComponent.this.isWindowCreated = true;

        // Make sure the JOGL Context is current
        GLContext ctx = drawable.getContext();
        if (!ctx.isCurrent()) {
          ctx.makeCurrent();
        }

        // Init SVTK OpenGL RenderWindow
        svtkAbstractJoglComponent.this.glRenderWindow.SetMapped(1);
        svtkAbstractJoglComponent.this.glRenderWindow.SetPosition(0, 0);
        svtkAbstractJoglComponent.this.setSize(drawable.getSurfaceWidth(), drawable.getSurfaceHeight());
        svtkAbstractJoglComponent.this.glRenderWindow.OpenGLInit();
      }

      public void reshape(GLAutoDrawable drawable, int x, int y, int width, int height) {
        svtkAbstractJoglComponent.this.setSize(width, height);
      }

      public void display(GLAutoDrawable drawable) {
        svtkAbstractJoglComponent.this.inRenderCall = true;
        svtkAbstractJoglComponent.this.glRenderWindow.Render();
        svtkAbstractJoglComponent.this.inRenderCall = false;
      }

      public void dispose(GLAutoDrawable drawable) {
        svtkAbstractJoglComponent.this.Delete();
        svtkObject.JAVA_OBJECT_MANAGER.gc(false);
      }
    };

    // Bind the interactor forwarder
    svtkInteractorForwarder forwarder = this.getInteractorForwarder();
    this.uiComponent.addMouseListener(forwarder);
    this.uiComponent.addMouseMotionListener(forwarder);
    this.uiComponent.addMouseWheelListener(forwarder);
    this.uiComponent.addKeyListener(forwarder);

    // Make sure when SVTK internally request a Render, the Render get
    // properly triggered
    renderWindowToUse.AddObserver("WindowFrameEvent", this, "Render");
    renderWindowToUse.GetInteractor().AddObserver("RenderEvent", this, "Render");
    renderWindowToUse.GetInteractor().SetEnableRender(false);
  }

  public T getComponent() {
    return this.uiComponent;
  }

  /**
   * Render the internal component
   */
  public void Render() {
    // Make sure we can render
    if (!inRenderCall) {
      this.uiComponent.repaint();
    }
  }

  /**
   * @return true if the graphical component has been properly set and
   * operation can be performed on it.
   */
  public boolean isWindowSet() {
    return this.isWindowCreated;
  }
}
