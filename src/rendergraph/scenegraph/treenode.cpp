#include "rendergraph/treenode.h"

using namespace rendergraph;

void TreeNode::setUsePreprocess(bool value) {
    backendNode()->setFlag(QSGNode::UsePreprocess, value);
}

TreeNode::~TreeNode() {
    m_pFirstChild.release();
    m_pNextSibling.release();
}

void TreeNode::onAppendChildNode(TreeNode* pChild) {
    backendNode()->appendChildNode(pChild->backendNode());
}

void TreeNode::onRemoveChildNode(TreeNode* pChild) {
    backendNode()->removeChildNode(pChild->backendNode());
}

void TreeNode::onRemoveAllChildNodes() {
    backendNode()->removeAllChildNodes();
}
