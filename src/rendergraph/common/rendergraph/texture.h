#pragma once

#include <QImage>
#include <memory>

#include "backend/basetexture.h"
#include "rendergraph/context.h"

namespace rendergraph {
class Texture;
} // namespace rendergraph

class rendergraph::Texture {
  public:
    Texture(Context* pContext, const QImage& image);

    // The image has to be same size as the image passed on construction.
    // For optimal performance the image should have format QImage::Format_RGBA8888_Premultiplied.
    // TODO: implement for scenegraph backend.
    void setData(const QImage& image);

    BaseTexture* backendTexture() const {
        return m_pTexture.get();
    }

    // used by Material::compare
    qint64 comparisonKey() const;

  private:
    const std::unique_ptr<BaseTexture> m_pTexture{};
};
