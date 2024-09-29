#pragma once

#include <QColor>
#include <memory>

#include "rendergraph/context.h"
#include "rendergraph/geometrynode.h"
#include "util/class.h"
#include "waveform/renderers/waveformrendererabstract.h"

class QDomNode;
class SkinContext;

namespace allshader {
class WaveformRendererPreroll;
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
            rendergraph::Context context,
            QColor color,
            ::WaveformRendererAbstract::PositionSource type =
                    ::WaveformRendererAbstract::Play)
            : WaveformRendererPreroll(waveformWidget, type) {
        m_context = context;
        m_color = color;
    }
    ~WaveformRendererPreroll() override;

    // Pure virtual from WaveformRendererAbstract, not used
    void draw(QPainter* painter, QPaintEvent* event) override final;

    void setup(const QDomNode& node, const SkinContext& context) override;

    // Virtual for rendergraph::Node
    void preprocess() override;

    void resize(const QRectF& geometry) override {
        m_geometry = geometry;
    }

  private:
    QColor m_color;
    rendergraph::Context m_context;
    float m_markerBreadth{};
    float m_markerLength{};
    bool m_isSlipRenderer;
    QRectF m_geometry;

    bool preprocessInner();

    DISALLOW_COPY_AND_ASSIGN(WaveformRendererPreroll);
};
