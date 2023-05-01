#pragma once

#include "waveform/renderers/glwaveformrenderer.h"
#include "waveform/widgets/waveformwidgetabstract.h"
#include "widget/wglwaveformwidget.h"

QT_FORWARD_DECLARE_CLASS(QString)

/// GLWaveformWidgetAbstract is a WaveformWidgetAbstract & WGLWidget. Any added
/// renderers derived from GLWaveformRenderer can implement a virtual method
/// onInitializeGL, which will be called from GLWaveformRenderer::initializeGL
/// (which overrides WGLWidget::initializeGL). This can be used for initialization
/// that must be deferred until the GL context has been initialized and that can't
/// be done in the constructor.
/// Note: WGLWaveformWidget is WGLWidget with added event forwarding when used with
/// OpenGLWindow
class GLWaveformWidgetAbstract : public WaveformWidgetAbstract, public WGLWaveformWidget {
  public:
    GLWaveformWidgetAbstract(const QString& group, QWidget* parent);

    WGLWidget* getGLWidget() override {
        return this;
    }

  protected:
#ifdef MIXXX_USE_QOPENGL
    void renderGL() override {
        // Called by OpenGLWindow to avoid flickering on resize.
        // The static_cast of this in required to avoid ambiguity
        // as method render is in both base classes.
        static_cast<WaveformWidgetAbstract*>(this)->render();
    }
#endif
#if !defined(QT_NO_OPENGL) && !defined(QT_OPENGL_ES_2)
    void initializeGL() override {
        for (auto renderer : std::as_const(m_rendererStack)) {
            auto glRenderer = dynamic_cast<GLWaveformRenderer*>(renderer);
            if (glRenderer) {
                glRenderer->onInitializeGL();
            }
        }
    }

#endif // !defined(QT_NO_OPENGL) && !defined(QT_OPENGL_ES_2)
};
