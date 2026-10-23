#include "label.hpp"

#include <algorithm>
#include <numbers>

#include <QCache>
#include <QFontMetricsF>
#include <QPainterPath>

namespace {

QFont makeFont(const Label& l, const QFont& base) {
    QFont f = base;
    if (!l.family.isEmpty())
        f.setFamily(l.family);
    f.setPixelSize(std::max(1, l.pixelSize));
    f.setBold(l.bold);
    return f;
}

QPointF alignedTopLeft(const QRectF& box, const QSizeF& textSize, Qt::Alignment align) {
    QPointF p = box.topLeft();
    if (align & Qt::AlignHCenter)
        p.setX(box.center().x() - textSize.width() * 0.5);
    else if (align & Qt::AlignRight)
        p.setX(box.right() - textSize.width());

    if (align & Qt::AlignVCenter)
        p.setY(box.center().y() - textSize.height() * 0.5);
    else if (align & Qt::AlignBottom)
        p.setY(box.bottom() - textSize.height());

    return p;
}

struct CachedGlyphPath {
    QPainterPath path; // baseline at (0,0)
    QRectF tight;
};

QString cacheKeyFor(const QString& text, const QFont& font) {
    // QFont::toString() includes many style properties, but it isn't stable across platforms.
    // Use explicit fields to keep cache hits consistent within the same runtime.
    return QString("%1|%2|%3|%4|%5|%6")
               .arg(font.family())
               .arg(font.pixelSize())
               .arg(font.weight())
               .arg(font.italic() ? 1 : 0)
               .arg(font.underline() ? 1 : 0)
               .arg(text);
}

CachedGlyphPath* getOrBuildCached(const QString& text, const QFont& font) {
    static QCache<QString, CachedGlyphPath> cache(512);
    const QString key = cacheKeyFor(text, font);

    if (auto* found = cache.object(key))
        return found;

    auto* entry = new CachedGlyphPath();
    const QFontMetricsF fm(font);
    entry->tight = fm.boundingRect(text);
    entry->path.addText(QPointF(0.0, 0.0), font, text);
    cache.insert(key, entry, 1);
    return entry;
}

} // namespace

void Label::drawInRect(QPainter* painter, const QRectF& rect, Qt::Alignment align, float rotationRadians) const {
    if (!painter || text.isEmpty())
        return;

    painter->save();
    painter->setRenderHint(QPainter::TextAntialiasing, true);
    painter->setRenderHint(QPainter::Antialiasing, true);

    const QFont f = makeFont(*this, painter->font());
    painter->setFont(f);

    CachedGlyphPath* cached = getOrBuildCached(text, f);
    const QRectF tight = cached->tight;
    const QSizeF size(std::max(0.0, tight.width()), std::max(0.0, tight.height()));
    const QPointF topLeft = alignedTopLeft(rect, size, align);

    // Convert to baseline position for addText().
    const QPointF baseline(topLeft.x() - tight.left(), topLeft.y() - tight.top());

    if (rotationRadians != 0.0f) {
        painter->translate(rect.center());
        painter->rotate(rotationRadians * 180.0f / std::numbers::pi_v<float>);
        painter->translate(-rect.center());
    }

    painter->translate(baseline);

    if (strokeWidth > 0.0f) {
        QPen pen(stroke, strokeWidth);
        pen.setJoinStyle(Qt::RoundJoin);
        painter->setPen(pen);
        painter->setBrush(Qt::NoBrush);
        painter->drawPath(cached->path);
    }

    painter->setPen(Qt::NoPen);
    painter->setBrush(fill);
    painter->drawPath(cached->path);

    painter->restore();
}

void Label::drawAt(QPainter* painter, const QPointF& pos, Qt::Alignment align, float rotationRadians) const {
    // Treat `pos` as the anchor for the given alignment.
    const QRectF r(pos.x() - 10000.0, pos.y() - 10000.0, 20000.0, 20000.0);
    drawInRect(painter, r, align, rotationRadians);
}
