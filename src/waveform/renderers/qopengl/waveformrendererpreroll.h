#pragma once

#include <QColor>
#include <QOpenGLShaderProgram>

#include "util/class.h"
#include "waveform/renderers/qopengl/waveformrenderer.h"

class QDomNode;
class SkinContext;

namespace qopengl {
class WaveformRendererPreroll;
}

class qopengl::WaveformRendererPreroll : public qopengl::WaveformRenderer {
  public:
    explicit WaveformRendererPreroll(WaveformWidgetRenderer* waveformWidgetRenderer);
    ~WaveformRendererPreroll() override;

    void setup(const QDomNode& node, const SkinContext& context) override;
    void renderGL() override;
    void initializeGL() override;

  private:
    QColor m_color;
    QVector<float> m_vertices;
    QOpenGLShaderProgram m_shaderProgram;

    DISALLOW_COPY_AND_ASSIGN(WaveformRendererPreroll);
};
