#pragma once

// #include "shaders/textureshader.h"
// #include "shaders/vinylqualityshader.h"
// #include "util/opengltexture2d.h"
#include "widget/wspinnybase.h"

namespace rendergraph {
class Engine;
class Context;
class GeometryNode;
} // namespace rendergraph

// class QOpenGLTexture;

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
    // void drawTexture(QOpenGLTexture* pTexture);
    // void cleanupGL();
    void updateTextures();

    void setupVinylSignalQuality() override;
    void updateVinylSignalQualityImage(
            const QColor& qual_color, const unsigned char* data) override;
    void drawVinylQuality();

    std::unique_ptr<rendergraph::Engine> m_pEngine;

    rendergraph::GeometryNode* m_pLoadedCoverNode;
    rendergraph::GeometryNode* m_pFgNode;
    // mixxx::TextureShader m_textureShader;
    // mixxx::VinylQualityShader m_vinylQualityShader;
    // OpenGLTexture2D m_bgTexture;
    // OpenGLTexture2D m_maskTexture;
    // OpenGLTexture2D m_fgTextureScaled;
    // OpenGLTexture2D m_ghostTextureScaled;
    // OpenGLTexture2D m_loadedCoverTextureScaled;
    // OpenGLTexture2D m_qTexture;
    QColor m_vinylQualityColor;
    bool m_textureUpdateNeeded{true};
};
