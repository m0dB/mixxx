#pragma once

#include <QColor>
#include <memory>

#include "rendergraph/geometrynode.h"
#include "rendergraph/opacitynode.h"
#include "util/class.h"
#include "util/performancetimer.h"
#include "waveform/renderers/waveformrendererabstract.h"

class ControlProxy;
class QDomNode;
class SkinContext;

namespace allshader {
class WaveformRendererEndOfTrack;
}

class allshader::WaveformRendererEndOfTrack final
        : public ::WaveformRendererAbstract,
          public rendergraph::GeometryNode {
  public:
    explicit WaveformRendererEndOfTrack(
            WaveformWidgetRenderer* waveformWidget);

    // Pure virtual from WaveformRendererAbstract, not used
    void draw(QPainter* painter, QPaintEvent* event) override final;

    void setup(const QDomNode& node, const SkinContext& context) override;

    bool init() override;

    // Virtual for rendergraph::Node
    void preprocess() override;
    bool isSubtreeBlocked() const override;

  private:
    static constexpr float positionArray[] = {-1.f, -1.f, 1.f, -1.f, -1.f, 1.f, 1.f, 1.f};
    static constexpr float verticalGradientArray[] = {1.f, 1.f, -1.f, -1.f};
    static constexpr float horizontalGradientArray[] = {-1.f, 1.f, -1.f, 1.f};

    std::unique_ptr<ControlProxy> m_pEndOfTrackControl;
    std::unique_ptr<ControlProxy> m_pTimeRemainingControl;

    QColor m_color;
    PerformanceTimer m_timer;

    DISALLOW_COPY_AND_ASSIGN(WaveformRendererEndOfTrack);
};
