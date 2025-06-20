#include "shaders/textureshader.h"

using namespace mixxx;

void TextureShader::init() {
    loadRendergraphFiles("texture.vert", "texture.frag");

    m_matrixLocation = uniformLocation("ubuf.matrix");
    m_positionLocation = attributeLocation("position");
    m_texcoordLocation = attributeLocation("texcoord");
    m_textureLocation = uniformLocation("texture1");
}
