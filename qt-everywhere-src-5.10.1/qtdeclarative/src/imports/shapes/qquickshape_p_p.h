/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the QtQuick module of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 3 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL3 included in the
** packaging of this file. Please review the following information to
** ensure the GNU Lesser General Public License version 3 requirements
** will be met: https://www.gnu.org/licenses/lgpl-3.0.html.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 2.0 or (at your option) the GNU General
** Public license version 3 or any later version approved by the KDE Free
** Qt Foundation. The licenses are as published by the Free Software
** Foundation and appearing in the file LICENSE.GPL2 and LICENSE.GPL3
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-2.0.html and
** https://www.gnu.org/licenses/gpl-3.0.html.
**
** $QT_END_LICENSE$
**
****************************************************************************/

#ifndef QQUICKSHAPE_P_P_H
#define QQUICKSHAPE_P_P_H

//
//  W A R N I N G
//  -------------
//
// This file is not part of the Qt API.  It exists purely as an
// implementation detail.  This header file may change from version to
// version without notice, or even be removed.
//
// We mean it.
//

#include "qquickshape_p.h"
#include <QtQuick/private/qquickitem_p.h>
#include <QPainterPath>
#include <QColor>
#include <QBrush>
#include <QElapsedTimer>
#include <private/qopenglcontext_p.h>

QT_BEGIN_NAMESPACE

class QSGPlainTexture;

class QQuickAbstractPathRenderer
{
public:
    enum Flag {
        SupportsAsync = 0x01
    };
    Q_DECLARE_FLAGS(Flags, Flag)

    enum FillGradientType { NoGradient = 0, LinearGradient, RadialGradient, ConicalGradient };
    struct GradientDesc { // can fully describe a linear/radial/conical gradient
        QGradientStops stops;
        QQuickShapeGradient::SpreadMode spread;
        QPointF a; // start (L) or center point (R/C)
        QPointF b; // end (L) or focal point (R)
        qreal v0; // center radius (R) or start angle (C)
        qreal v1; // focal radius (R)
    };

    virtual ~QQuickAbstractPathRenderer() { }

    // Gui thread
    virtual void beginSync(int totalCount) = 0;
    virtual void endSync(bool async) = 0;
    virtual void setAsyncCallback(void (*)(void *), void *) { }
    virtual Flags flags() const { return 0; }
    virtual void setPath(int index, const QQuickPath *path) = 0;
    virtual void setStrokeColor(int index, const QColor &color) = 0;
    virtual void setStrokeWidth(int index, qreal w) = 0;
    virtual void setFillColor(int index, const QColor &color) = 0;
    virtual void setFillRule(int index, QQuickShapePath::FillRule fillRule) = 0;
    virtual void setJoinStyle(int index, QQuickShapePath::JoinStyle joinStyle, int miterLimit) = 0;
    virtual void setCapStyle(int index, QQuickShapePath::CapStyle capStyle) = 0;
    virtual void setStrokeStyle(int index, QQuickShapePath::StrokeStyle strokeStyle,
                                qreal dashOffset, const QVector<qreal> &dashPattern) = 0;
    virtual void setFillGradient(int index, QQuickShapeGradient *gradient) = 0;

    // Render thread, with gui blocked
    virtual void updateNode() = 0;
};

Q_DECLARE_OPERATORS_FOR_FLAGS(QQuickAbstractPathRenderer::Flags)

struct QQuickShapeStrokeFillParams
{
    QQuickShapeStrokeFillParams();

    QColor strokeColor;
    qreal strokeWidth;
    QColor fillColor;
    QQuickShapePath::FillRule fillRule;
    QQuickShapePath::JoinStyle joinStyle;
    int miterLimit;
    QQuickShapePath::CapStyle capStyle;
    QQuickShapePath::StrokeStyle strokeStyle;
    qreal dashOffset;
    QVector<qreal> dashPattern;
    QQuickShapeGradient *fillGradient;
};

class QQuickShapePathPrivate : public QQuickPathPrivate
{
    Q_DECLARE_PUBLIC(QQuickShapePath)

public:
    enum Dirty {
        DirtyPath = 0x01,
        DirtyStrokeColor = 0x02,
        DirtyStrokeWidth = 0x04,
        DirtyFillColor = 0x08,
        DirtyFillRule = 0x10,
        DirtyStyle = 0x20,
        DirtyDash = 0x40,
        DirtyFillGradient = 0x80,

        DirtyAll = 0xFF
    };

    QQuickShapePathPrivate();

    void _q_pathChanged();
    void _q_fillGradientChanged();

    static QQuickShapePathPrivate *get(QQuickShapePath *p) { return p->d_func(); }

    int dirty;
    QQuickShapeStrokeFillParams sfp;
};

class QQuickShapePrivate : public QQuickItemPrivate
{
    Q_DECLARE_PUBLIC(QQuickShape)

public:
    QQuickShapePrivate();
    ~QQuickShapePrivate();

    void createRenderer();
    QSGNode *createNode();
    void sync();

    void _q_shapePathChanged();
    void setStatus(QQuickShape::Status newStatus);

    static QQuickShapePrivate *get(QQuickShape *item) { return item->d_func(); }

    static void asyncShapeReady(void *data);

    bool spChanged;
    int effectRefCount;
    QQuickShape::RendererType rendererType;
    bool async;
    QQuickShape::Status status;
    QQuickAbstractPathRenderer *renderer;
    QVector<QQuickShapePath *> sp;
    bool enableVendorExts;
    bool syncTimingActive = false;
    int syncTimingTotalDirty;
    int syncTimeCounter = 0;
    QElapsedTimer syncTimer;
};

#if QT_CONFIG(opengl)

class QQuickShapeGradientCache : public QOpenGLSharedResource
{
public:
    struct Key {
        Key(const QGradientStops &stops, QQuickShapeGradient::SpreadMode spread)
            : stops(stops), spread(spread)
        { }
        QGradientStops stops;
        QQuickShapeGradient::SpreadMode spread;
        bool operator==(const Key &other) const
        {
            return spread == other.spread && stops == other.stops;
        }
    };

    QQuickShapeGradientCache(QOpenGLContext *context) : QOpenGLSharedResource(context->shareGroup()) { }
    ~QQuickShapeGradientCache();

    void invalidateResource() override;
    void freeResource(QOpenGLContext *) override;

    QSGTexture *get(const Key &grad);

    static QQuickShapeGradientCache *currentCache();

private:
    QHash<Key, QSGPlainTexture *> m_cache;
};

inline uint qHash(const QQuickShapeGradientCache::Key &v, uint seed = 0)
{
    uint h = seed + v.spread;
    for (int i = 0; i < 3 && i < v.stops.count(); ++i)
        h += v.stops[i].second.rgba();
    return h;
}

#endif // QT_CONFIG(opengl)

QT_END_NAMESPACE

#endif
