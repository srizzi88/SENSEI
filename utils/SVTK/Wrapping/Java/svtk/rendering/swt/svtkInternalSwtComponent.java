package svtk.rendering.swt;

import org.eclipse.swt.SWT;
import org.eclipse.swt.widgets.Composite;
import org.eclipse.swt.widgets.Event;
import org.eclipse.swt.widgets.Listener;

import com.jogamp.opengl.GLAutoDrawable;
import com.jogamp.opengl.GLCapabilities;
import com.jogamp.opengl.GLProfile;
import com.jogamp.opengl.GLRunnable;
import com.jogamp.opengl.swt.GLCanvas;

import svtk.svtkObject;

/**
 * @author    Joachim Pouderoux - joachim.pouderoux@kitware.com, Kitware SAS 2012
 * @copyright This work was supported by CEA/CESTA
 *            Commissariat a l'Energie Atomique et aux Energies Alternatives,
 *            15 avenue des Sablieres, CS 60001, 33116 Le Barp, France.
 */
public class svtkInternalSwtComponent extends GLCanvas implements Listener {

  private svtkSwtComponent parent;

  public static GLCapabilities GetGLCapabilities() {
    GLCapabilities caps;
    caps = new GLCapabilities(GLProfile.get(GLProfile.GL2GL3));
    caps.setDoubleBuffered(true);
    caps.setHardwareAccelerated(true);
    caps.setSampleBuffers(false);
    caps.setNumSamples(4);

    return caps;
  }

  public svtkInternalSwtComponent(svtkSwtComponent parent, Composite parentComposite) {

    super(parentComposite, SWT.NO_BACKGROUND, GetGLCapabilities(), null);
    this.parent = parent;

    svtkSwtInteractorForwarderDecorator forwarder = (svtkSwtInteractorForwarderDecorator) this.parent
      .getInteractorForwarder();

    this.addMouseListener(forwarder);
    this.addKeyListener(forwarder);
    this.addMouseMoveListener(forwarder);
    this.addMouseTrackListener(forwarder);
    this.addMouseWheelListener(forwarder);

    this.addListener(SWT.Paint, this);
    this.addListener(SWT.Close, this);
    this.addListener(SWT.Dispose, this);
    this.addListener(SWT.Resize, this);

    this.IntializeRenderWindow();
  }

  protected void IntializeRenderWindow() {

    // setCurrent(); // need to be done so SetWindowIdFromCurrentContext can
    // get the current context!
    // Context is not created until the first draw call. The renderer isn't
    // initialized until the context is
    // present.
    invoke(false, new GLRunnable() {

      @Override
      public boolean run(GLAutoDrawable arg0) {
        // This makes this thread (should be the main thread) current
        getContext().makeCurrent();
        parent.getRenderWindow().InitializeFromCurrentContext();
        // Swapping buffers is handled by the svtkSwtComponent
        parent.getRenderWindow().SwapBuffersOff();
        return false;
      }
    });

    // Swap buffers to trigger context creation
    swapBuffers();
    setAutoSwapBufferMode(false);
  }

  @Override
  public void update() {
    super.update();
    if (isRealized()) {
      parent.Render();
    }
  }

  @Override
  public void dispose() {
    this.removeListener(SWT.Paint, this);
    this.removeListener(SWT.Close, this);
    this.removeListener(SWT.Dispose, this);
    this.removeListener(SWT.Resize, this);

    if (getContext().isCurrent()) {
      getContext().release();
    }
    super.dispose();
  }

  @Override
  public void handleEvent(Event event) {
    switch (event.type) {
    case SWT.Paint:
      if (isRealized()) {
        parent.Render();
      }
      break;
    case SWT.Dispose:
      parent.Delete();
      svtkObject.JAVA_OBJECT_MANAGER.gc(false);
      break;
    case SWT.Close:
      // System.out.println("closing");
      break;
    case SWT.Resize:
      parent.setSize(getClientArea().width, getClientArea().height);
      break;
    }
  }
}
