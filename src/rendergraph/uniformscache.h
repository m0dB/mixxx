#pragma once

#include <QByteArray>
#include <QColor>
#include <QMatrix4x4>
#include <QVector4D>

#include "rendergraph/types.h"

namespace rendergraph {
class UniformSet;
class UniformsCache;
} // namespace rendergraph

class rendergraph::UniformsCache {
  public:
    UniformsCache(const UniformSet& uniformSet);
    ~UniformsCache();

    template<typename T>
    void set(int uniformIndex, const T& value) {
        set(uniformIndex, typeOf<T>(), static_cast<const void*>(&value), sizeOf(typeOf<T>()));
    }

    template<typename T>
    T get(int uniformIndex) const {
        T value;
        get(uniformIndex, typeOf<T>(), static_cast<void*>(&value), sizeof(T));
        return value;
    }
    Type type(int uniformIndex) const {
        return m_infos[uniformIndex].m_type;
    }

    const char* data() const {
        return m_byteArray.data();
    }
    qsizetype size() const {
        return m_byteArray.size();
    }
    int count() const {
        return m_infos.size();
    }

  private:
    void set(int uniformIndex, Type type, const void* ptr, int size);
    void get(int uniformIndex, Type type, void* ptr, int size) const;

    struct Info {
        const Type m_type;
        const int m_offset;
    };

    std::vector<Info> m_infos;
    QByteArray m_byteArray;
};

template<>
inline void rendergraph::UniformsCache::set<QColor>(int uniformIndex, const QColor& color) {
    set(uniformIndex, QVector4D{color.redF(), color.greenF(), color.blueF(), color.alphaF()});
}

template<>
inline void rendergraph::UniformsCache::set<QMatrix4x4>(
        int uniformIndex, const QMatrix4x4& matrix) {
    set(uniformIndex, typeOf<QMatrix4x4>(), matrix.constData(), sizeOf(typeOf<QMatrix4x4>()));
}
