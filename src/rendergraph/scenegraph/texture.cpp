#include "rendergraph/texture.h"

#include "rendergraph/assert.h"
#include "rendergraph/context.h"

using namespace rendergraph;

Texture::Texture(Context& context, const QImage& image)
        : m_pTexture(context.window()->createTextureFromImage(image)) {
    DEBUG_ASSERT(!m_pTexture->textureSize().isNull());
}

qint64 Texture::comparisonKey() const {
    return m_pTexture->comparisonKey();
}
