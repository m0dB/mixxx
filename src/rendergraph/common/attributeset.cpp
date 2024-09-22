#include "rendergraph/attributeset.h"

using namespace rendergraph;

AttributeSet::AttributeSet(std::initializer_list<Attribute> list,
        const std::vector<QString>& names)
        : BaseAttributeSet(list, names) {
}

const std::vector<Attribute>& AttributeSet::attributes() const {
    return m_attributes;
}
