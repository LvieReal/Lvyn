#pragma once

#include <QColor>
#include <QPointF>
#include <QQuickItem>
#include <QSGGeometryNode>
#include <QSGMaterial>
#include <QSGMaterialShader>
#include <QVector2D>
#include <QVector4D>
#include <QMatrix4x4>

#include <cstring>

#include "settings.hpp"

class GameManager;

class GridMaterial : public QSGMaterial {
public:
    QVector2D viewPos;
    float zoom = 1.0f;
    QVector2D viewportSize;
    QVector2D arenaSize;

    float gridSize = static_cast<float>(Settings::arenaGridSize);
    float gridLineWidth = Settings::arenaGridLineWidth;

    QVector4D arenaColor;
    QVector4D backgroundColor;
    QVector4D minorGridColor;
    QVector4D majorGridColor;

    QSGMaterialType* type() const override {
        static QSGMaterialType type;
        return &type;
    }

    QSGMaterialShader* createShader(QSGRendererInterface::RenderMode) const override;
};

class GridShader : public QSGMaterialShader {
public:
    GridShader() {
        setShaderFileName(VertexStage, ":/assets/shaders/grid.vert.qsb");
        setShaderFileName(FragmentStage, ":/assets/shaders/grid.frag.qsb");
    }

    bool updateUniformData(RenderState& state, QSGMaterial* newMaterial, QSGMaterial*) override {
        auto* mat = static_cast<GridMaterial*>(newMaterial);

        QByteArray* buf = state.uniformData();
        // Must match the std140 uniform block size reported by qsb reflection.
        // If qt_Matrix is not written, gl_Position becomes garbage and nothing renders.
        constexpr int kBufSizeBytes = 176;
        if (buf->size() != kBufSizeBytes)
            buf->resize(kBufSizeBytes);

        // Always write the matrix: on the first frame the data buffer is uninitialized.
        const QMatrix4x4 m = state.combinedMatrix();
        std::memcpy(buf->data(), m.constData(), 16 * sizeof(float));

        float* data = reinterpret_cast<float*>(buf->data());

        // Qt reserves the first 16 floats for qt_Matrix.
        int i = 16;

        data[i++] = mat->viewPos.x();
        data[i++] = mat->viewPos.y();
        data[i++] = mat->zoom;
        data[i++] = 0.0f; // pad

        data[i++] = mat->viewportSize.x();
        data[i++] = mat->viewportSize.y();

        data[i++] = mat->arenaSize.x();
        data[i++] = mat->arenaSize.y();

        // gridParams (vec2)
        data[i++] = mat->gridSize;
        data[i++] = mat->gridLineWidth;

        // std140 padding: the next vec4 must start at a 16-byte boundary, so insert 2 floats.
        data[i++] = 0.0f;
        data[i++] = 0.0f;

        // vec4 colors
        data[i++] = mat->arenaColor.x();
        data[i++] = mat->arenaColor.y();
        data[i++] = mat->arenaColor.z();
        data[i++] = mat->arenaColor.w();

        data[i++] = mat->backgroundColor.x();
        data[i++] = mat->backgroundColor.y();
        data[i++] = mat->backgroundColor.z();
        data[i++] = mat->backgroundColor.w();

        data[i++] = mat->minorGridColor.x();
        data[i++] = mat->minorGridColor.y();
        data[i++] = mat->minorGridColor.z();
        data[i++] = mat->minorGridColor.w();

        data[i++] = mat->majorGridColor.x();
        data[i++] = mat->majorGridColor.y();
        data[i++] = mat->majorGridColor.z();
        data[i++] = mat->majorGridColor.w();

        return true;
    }
};

class GridItem : public QQuickItem {
public:
    explicit GridItem(GameManager* gameManager = nullptr) : m_gameManager(gameManager) {
        setFlag(ItemHasContents, true);
    }

    void setGameManager(GameManager* gameManager) {
        m_gameManager = gameManager;
    }

protected:
    QSGNode* updatePaintNode(QSGNode* oldNode, UpdatePaintNodeData*) override;

private:
    GameManager* m_gameManager = nullptr; // non-owning
};
