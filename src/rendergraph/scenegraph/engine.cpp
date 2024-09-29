#include "rendergraph/engine.h"

#include <cassert>

using namespace rendergraph;

void Engine::addToEngine(TreeNode* pNode) {
    assert(pNode->engine() == nullptr);

    pNode->setEngine(this);
    m_pInitializeNodes.push_back(pNode);
    pNode = pNode->firstChild();
    while (pNode) {
        if (pNode->engine() != this) {
            addToEngine(pNode);
        }
        pNode = pNode->nextSibling();
    }
}

Engine::~Engine() {
    m_pTopNode.release();
}

void Engine::render() {
    assert(false && "should not be called for scenegraph, rendering is handled by Qt");
}

void Engine::preprocess() {
    assert(false && "should not be called for scenegraph, preprocess is handled by Qt");
}

void Engine::initialize() {
    for (auto pNode : m_pInitializeNodes) {
        pNode->initialize();
    }
    m_pInitializeNodes.clear();
}

void Engine::resize(TreeNode* pNode, const QRectF& boundingRect) {
    pNode->resize(boundingRect);
    pNode = pNode->firstChild();
    while (pNode) {
        resize(pNode, boundingRect);
        pNode = pNode->nextSibling();
    }
}
