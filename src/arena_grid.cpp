#include "arena_grid.hpp"

#include <algorithm>

#include "game_manager.hpp"

QSGMaterialShader* GridMaterial::createShader(QSGRendererInterface::RenderMode) const {
    return new GridShader();
}

QSGNode* GridItem::updatePaintNode(QSGNode* oldNode, UpdatePaintNodeData*) {
    QSGGeometryNode* node = static_cast<QSGGeometryNode*>(oldNode);

    if (!node) {
        node = new QSGGeometryNode();

        auto* geom = new QSGGeometry(QSGGeometry::defaultAttributes_Point2D(), 4);
        geom->setDrawingMode(QSGGeometry::DrawTriangleStrip);

        node->setGeometry(geom);
        node->setFlag(QSGNode::OwnsGeometry);

        auto* material = new GridMaterial();
        node->setMaterial(material);
        node->setFlag(QSGNode::OwnsMaterial);
    }

    // Keep geometry in sync with item size (width/height can change at runtime).
    auto* geom = node->geometry();
    auto* v = geom->vertexDataAsPoint2D();
    v[0].set(0.0f, 0.0f);
    v[1].set(static_cast<float>(width()), 0.0f);
    v[2].set(0.0f, static_cast<float>(height()));
    v[3].set(static_cast<float>(width()), static_cast<float>(height()));
    node->markDirty(QSGNode::DirtyGeometry);

    auto* material = static_cast<GridMaterial*>(node->material());
    QPointF viewPos;
    float zoom = 1.0f;
    if (m_gameManager) {
        viewPos = m_gameManager->getViewPos();
        zoom = m_gameManager->getViewZoom();
    }
    material->viewPos = QVector2D(viewPos);
    material->zoom = std::max(0.001f, zoom);
    material->viewportSize = QVector2D(width(), height());
    material->arenaSize = QVector2D(static_cast<float>(Settings::arenaWidth), static_cast<float>(Settings::arenaHeight));
    material->gridSize = static_cast<float>(Settings::arenaGridSize);
    material->gridLineWidth = Settings::arenaGridLineWidth;

    material->arenaColor = QVector4D(
        Settings::arenaColor.redF(),
        Settings::arenaColor.greenF(),
        Settings::arenaColor.blueF(),
        Settings::arenaColor.alphaF()
    );
    material->backgroundColor = QVector4D(
        Settings::backgroundColor.redF(),
        Settings::backgroundColor.greenF(),
        Settings::backgroundColor.blueF(),
        Settings::backgroundColor.alphaF()
    );

    const QColor grid = Settings::arenaGridColor;
    material->minorGridColor = QVector4D(grid.redF(), grid.greenF(), grid.blueF(), grid.alphaF());
    material->majorGridColor = material->minorGridColor;

    node->markDirty(QSGNode::DirtyMaterial);
    return node;
};
