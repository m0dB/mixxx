#pragma once

#include "rendergraph/opengl/openglnode.h"
#include "waveform/renderers/waveformrendererabstract.h"

class WaveformWidgetRenderer;

namespace allshader {
class WaveformRenderer;
}

class allshader::WaveformRenderer : public ::WaveformRendererAbstract,
                                    public rendergraph::OpenGLNode {
  public:
    explicit WaveformRenderer(WaveformWidgetRenderer* widget);

    // Pure virtual from WaveformRendererAbstract
    // Renderers that use QPainter functionality implement this.
    // But as all classes derived from allshader::WaveformRenderer
    // will only use openGL functions (combining QPainter and
    // QOpenGLWindow has bad performance), we leave this empty.
    // Should never be called.
    void draw(QPainter* painter, QPaintEvent* event) override final;
};
