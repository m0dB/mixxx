#pragma once

// #include "shaders/vinylqualityshader.h"
#include "widget/wspinnybase.h"

namespace rendergraph {
class Engine;
class Context;
class Node;
class GeometryNode;
} // namespace rendergraph


class WSpinnyGLSL : public WSpinnyBase {
    Q_OBJECT
  public:
    WSpinnyGLSL(QWidget* parent,
            const QString& group,
            UserSettingsPointer pConfig,
            VinylControlManager* pVCMan,
            BaseTrackPlayer* pPlayer);
    ~WSpinnyGLSL() override;

    rendergraph::Context* getContext() const {
        return nullptr;
    }

  private:
    void draw() override;
    void coverChanged() override;

    void initializeGL() override;
    void paintGL() override;
    void resizeGL(int w, int h) override;
    void updateTextureNodes();
    void updateTextureNode(rendergraph::GeometryNode* pNode, const QImage& image);
    rendergraph::GeometryNode* createTextureNode(rendergraph::Node* pParentNode);

    void setupVinylSignalQuality() override;
    void updateVinylSignalQualityImage(
            const QColor& qual_color, const unsigned char* data) override;

    std::unique_ptr<rendergraph::Engine> m_pEngine;

    rendergraph::GeometryNode* m_pBgNode;
    rendergraph::GeometryNode* m_pLoadedCoverNode;
    rendergraph::GeometryNode* m_pFgNode;
    rendergraph::GeometryNode* m_pGhostNode;
    rendergraph::GeometryNode* m_pMaskNode;
    rendergraph::GeometryNode* m_pVinylQualityNode;
    QImage m_vinylQualityImage;
    bool m_bLoadedCoverNodeUpdatePending{};
    bool m_bDrawingVinylQuality{};
};
