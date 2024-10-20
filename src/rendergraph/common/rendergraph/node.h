#pragma once

#include "backend/basenode.h"
#include "rendergraph/treenode.h"

namespace rendergraph {
class Node;
} // namespace rendergraph

class rendergraph::Node : public rendergraph::BaseNode, public rendergraph::TreeNode {
  public:
    using rendergraph::TreeNode::appendChildNode;
    using rendergraph::TreeNode::firstChild;
    using rendergraph::TreeNode::lastChild;
    using rendergraph::TreeNode::nextSibling;
    using rendergraph::TreeNode::removeChildNode;
    Node();
};
