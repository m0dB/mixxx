#include "rendergraph/treenode.h"

using namespace rendergraph;

TreeNode::TreeNode(rendergraph::BaseNode* pBackendNode)
        : m_pBackendNode(pBackendNode) {
    m_pBackendNode->setFlag(QSGNode::OwnedByParent, false);
}

TreeNode::~TreeNode() {
    m_pFirstChild.release();
    m_pNextSibling.release();
}

void TreeNode::setUsePreprocess(bool value) {
    backendNode()->setFlag(QSGNode::UsePreprocess, value);
}

void TreeNode::onAppendChildNode(TreeNode* pChild) {
    auto pThisBackendNode = backendNode();
    auto pChildBackendNode = pChild->backendNode();

    pThisBackendNode->appendChildNode(pChildBackendNode);

    // backendNode()->appendChildNode(pChild->backendNode());
}

void TreeNode::onRemoveChildNode(TreeNode* pChild) {
    backendNode()->removeChildNode(pChild->backendNode());
}

void TreeNode::onRemoveAllChildNodes() {
    backendNode()->removeAllChildNodes();
}
