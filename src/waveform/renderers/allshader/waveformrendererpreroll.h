#pragma once

#include <QColor>
#include <memory>

#include "rendergraph/geometrynode.h"
#include "util/class.h"
#include "waveform/renderers/waveformrendererabstract.h"

class QDomNode;
class SkinContext;

namespace allshader {
class WaveformRendererPreroll;
}
namespace rendergraph {
class Context;
}

class allshader::WaveformRendererPreroll final
        : public ::WaveformRendererAbstract,
          public rendergraph::GeometryNode {
  public:
    explicit WaveformRendererPreroll(
            WaveformWidgetRenderer* waveformWidget,
            ::WaveformRendererAbstract::PositionSource type =
                    ::WaveformRendererAbstract::Play);
    explicit WaveformRendererPreroll(
            WaveformWidgetRenderer* waveformWidget,
            rendergraph::Context* pContext,
            QColor color,
            ::WaveformRendererAbstract::PositionSource type =
                    ::WaveformRendererAbstract::Play)
            : WaveformRendererPreroll(waveformWidget, type) {
        m_pContext = pContext;
        m_color = color;
    }
    ~WaveformRendererPreroll() override;

    // Pure virtual from WaveformRendererAbstract, not used
    void draw(QPainter* painter, QPaintEvent* event) override final;

    void setup(const QDomNode& node, const SkinContext& skinContext) override;

    // Virtual for rendergraph::Node
    void preprocess() override;

  private:
    QColor m_color;
    rendergraph::Context* m_pContext;
    float m_markerBreadth{};
    float m_markerLength{};
    bool m_isSlipRenderer;

    bool preprocessInner();

    DISALLOW_COPY_AND_ASSIGN(WaveformRendererPreroll);
};
