#pragma once

#include <QColor>
#include <QOpenGLShaderProgram>
#include <QTime>

#include "util/class.h"
#include "util/performancetimer.h"
#include "waveform/renderers/qopengl/waveformrenderer.h"

class ControlObject;
class ControlProxy;
class QDomNode;
class SkinContext;

namespace qopengl {
class WaveformRendererEndOfTrack;
}

class qopengl::WaveformRendererEndOfTrack : public qopengl::WaveformRenderer {
  public:
    explicit WaveformRendererEndOfTrack(
            WaveformWidgetRenderer* waveformWidget);
    ~WaveformRendererEndOfTrack() override;

    void setup(const QDomNode& node, const SkinContext& context) override;

    bool init() override;

    void initializeGL() override;
    void renderGL() override;

  private:
    void fillWithGradient(QColor color);

    ControlProxy* m_pEndOfTrackControl;
    ControlProxy* m_pTimeRemainingControl;
    QOpenGLShaderProgram m_shaderProgram;

    QColor m_color;
    PerformanceTimer m_timer;

    DISALLOW_COPY_AND_ASSIGN(WaveformRendererEndOfTrack);
};
