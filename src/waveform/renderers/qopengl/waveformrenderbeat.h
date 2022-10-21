#pragma once

#include <QColor>
#include <QOpenGLShaderProgram>

#include "util/class.h"
#include "waveform/renderers/qopengl/waveformrenderer.h"

class QDomNode;
class SkinContext;

namespace qopengl {
class WaveformRenderBeat;
}

class qopengl::WaveformRenderBeat : public qopengl::WaveformRenderer {
  public:
    explicit WaveformRenderBeat(WaveformWidgetRenderer* waveformWidget);
    ~WaveformRenderBeat() override;

    void setup(const QDomNode& node, const SkinContext& context) override;
    void renderGL() override;
    void initializeGL() override;

  private:
    QColor m_beatColor;
    QVector<float> m_beatLineVertices;
    QOpenGLShaderProgram m_shaderProgram;

    DISALLOW_COPY_AND_ASSIGN(WaveformRenderBeat);
};
