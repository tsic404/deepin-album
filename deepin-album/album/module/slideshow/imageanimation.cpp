﻿/*
* Copyright (C) 2019 ~ 2020 Deepin Technology Co., Ltd.
*
* Author: Deng jinhui<dengjinhui@uniontech.com>
*
* Maintainer: Deng jinhui <dengjinhui@uniontech.com>
*
* This program is free software: you can redistribute it and/or modify
* it under the terms of the GNU General Public License as published by
* the Free Software Foundation, either version 3 of the License, or
* any later version.
*
* This program is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
* GNU General Public License for more details.
*
* You should have received a copy of the GNU General Public License
* along with this program. If not, see <http://www.gnu.org/licenses/>.
*/
#include "imageanimation.h"
#include "unionimage.h"
#include "application.h"

#include <QDebug>
#include <QVBoxLayout>
#include <QPainter>
#include <QPointer>
#include <QSharedPointer>
#include <QTime>
#include <QTimer>
#include <QObject>
#include <QDesktopWidget>

#include <cmath>

float GaussFunction(double max, float mu, float sigma, float x)
{
    return static_cast<float>(max * std::exp(static_cast<double>(-(x - mu) * (x - mu) / 2 * sigma * sigma)));
}


class LoopQueue
{
public:
    LoopQueue(const QString &beginPath, const QStringList &list);
    inline const QString first()const;
    inline const QString second()const;
    inline int index() const;
    inline const QString next();
    inline const QString pre();
    inline const QString jumpTonext();
    inline const QString jumpTopre();
    inline const QString current();
    inline void changeOrder(bool order);
private:
    inline void AddIndex();
private:
    QVector<QString> loop_paths;
    bool loop_order;
    char m_padding[3]; //填充占位,使数据结构内存对齐
    int loop_pindex;
};

class ImageAnimationPrivate: public QWidget
{
public:
    enum AnimationType {
        FadeEffect = 0,             //图像1渐渐变淡,图像2渐渐显现
        BlindsEffect = 1,           //百叶窗效果
        FlipRightToLeft = 2,        //图像从右向左翻转
        OutsideToInside = 3,        //从外到内水平分割
        MoveLeftToRightEffect = 4,  //图像1从左至右退出可视区域,同时图像2从左至右进入可视区域
        MoveRightToLeftEffect = 5,  //图像1从左至右退出可视区域,同时图像2从左至右进入可视区域
        MoveBottomToUpEffect = 6,   //图像1从下至上退出可视区域,同时图像2从下至上进入可视区域
        MoveUpToBottomEffect = 7,   //图像1从上至下退出可视区域,同时图像2从上至下进入可视区域
        MoveBottomToLeftUpEffect = 8//图像1不动,同时图像2从右下到左上
    };
protected:
    ImageAnimationPrivate(ImageAnimation *qq = nullptr);
    ~ImageAnimationPrivate();
    void effectPainter(QPainter *painter, const QRect &rect);
    void forwardPainter(QPainter *painter, const QRect &rect);
    void retreatPainter(QPainter *painter, const QRect &rect);
    void keepStaticPainter(QPainter *painter, const QRect &rect);
    /**
    ****************************************************************************************************************
    Effects
    ****************************************************************************************************************
    */
    void fadeEffect(QPainter *painter, const QRect &rect, float factor, const QPixmap &pixmap1, const QPixmap &pixmap2);
    void blindsEffect(QPainter *painter, const QRect &rect, float factor, const QPixmap &pixmap1, const QPixmap &pixmap2);
    void flipRightToLeft(QPainter *painter, const QRect &rect, float factor, const QPixmap &pixmap1, const QPixmap &pixmap2);
    void outsideToInside(QPainter *painter, const QRect &rect, float factor, const QPixmap &pixmap1, const QPixmap &pixmap2);
    void moveLeftToRightEffect(QPainter *painter, const QRect &rect, float factor, const QPixmap &pixmap1, const QPixmap &pixmap2);
    void moveRightToLeftEffect(QPainter *painter, const QRect &rect, float factor, const QPixmap &pixmap1, const QPixmap &pixmap2);
    void moveBottomToUpEffect(QPainter *painter, const QRect &rect, float factor, const QPixmap &pixmap1, const QPixmap &pixmap2);
    void moveUpToBottomEffect(QPainter *painter, const QRect &rect, float factor, const QPixmap &pixmap1, const QPixmap &pixmap2);
    void moveBottomToLeftUpEffect(QPainter *painter, const QRect &rect, float factor, const QPixmap &pixmap1, const QPixmap &pixmap2);

    /**
    ****************************************************************************************************************
    */
private:
    float m_factor;                   //动画因子(0 - 1.0之间变化)
    float m_funval;
    QString m_imageName1;             //图片1路径名称
    QString m_imageName2;             //图片2路径名称
    QPixmap m_pixmap1;                //图片1
    QPixmap m_pixmap2;                //图片2
    AnimationType m_animationType;    //动画效果类型

public:
    float getFactor()               const
    {
        return m_factor;
    }
    QString getImageName1()         const
    {
        return m_imageName1;
    }
    QString getImageName2()         const
    {
        return m_imageName2;
    }
    QPixmap getPixmap1()            const
    {
        return m_pixmap1;
    }
    QPixmap getPixmap2()            const
    {
        return m_pixmap2;
    }
    AnimationType getAnimationType()const
    {
        return m_animationType;
    }

    void setPathList(const QString &first, const QStringList &list);
    void startAnimation();
    void stopAnimation();
    void startStatic();
    void stopStatic();

private:
    //设置动画因子
    inline void setFactor(float factor_bar)
    {
        m_factor = factor_bar;
    }

    //设置图片1+图片2路径名称并适应widget
    void setImage1(const QString &imageName1_bar);
    void setImage2(const QString &imageName2_bar);

    //设置图片1+图片2
    void setPixmap1(const QPixmap &pixmap1_bar)
    {
        m_pixmap1 = pixmap1_bar;
    }
    void setPixmap2(const QPixmap &pixmap2_bar)
    {
        m_pixmap2 = pixmap2_bar;
    }
    //设置动画类型
    void setAnimationType(const AnimationType &animationType_bar)
    {
        m_animationType = animationType_bar;
    }
private:
    char m_padding2[4];                 //填充占位,使数据结构内存对齐
    QSharedPointer<LoopQueue> queue;

    QPointer<QTimer> m_animationTimer;
    QPointer<QTimer> m_staticTimer;
    QPointer<QRect> m_rect;
    QPoint centrePoint;
//    int beginX;
//    int beginY;
//    int finalX;
//    int finalY;

    ImageAnimation *const q_ptr;
    Q_DECLARE_PUBLIC(ImageAnimation)
};

/**
 ****************************************************************************************************************
 *  LoopQueue
 ****************************************************************************************************************
 */

LoopQueue::LoopQueue(const QString &beginPath, const QStringList &list): loop_order(true), loop_pindex(1)
{
    Q_UNUSED(m_padding);
    loop_paths.clear();
    int newfirst = list.indexOf(beginPath);
    QVector<QString> temp;
    QList<QString>::const_iterator i = list.begin();
    for (int j = 0; i != list.end() && j < newfirst; i++, j++)
        temp.append(*i);
    for (; i != list.end(); i++)
        loop_paths.append(*i);
    loop_paths.append(temp);
}

const QString LoopQueue::first() const
{
    return loop_paths[0];
}

const QString LoopQueue::second() const
{
    return loop_paths[1];
}

int LoopQueue::index() const
{
    return loop_pindex;
}

const QString LoopQueue::next()
{
    if (loop_pindex + 1 >= loop_paths.size()) {
        return loop_paths.first();
    } else {
        return loop_paths[loop_pindex + 1];
    }
}

const QString LoopQueue::pre()
{
    if (loop_pindex - 1 < 0) {
        return loop_paths.last();
    } else {
        return loop_paths[loop_pindex - 1];
    }
}

const QString LoopQueue::jumpTonext()
{
    AddIndex();
    return loop_paths[loop_pindex];
}

const QString LoopQueue::jumpTopre()
{
    changeOrder(false);
    AddIndex();
    changeOrder(true); // 没有需求逆序播放
    return loop_paths[loop_pindex];
}

const QString LoopQueue::current()
{
    return loop_paths[loop_pindex];
}

void LoopQueue::changeOrder(bool order)
{
    loop_order = order;
}

void LoopQueue::AddIndex()
{
    if (loop_order) {
        loop_pindex++;
        if (loop_pindex >= loop_paths.size()) {
            loop_pindex = 0;
        }
    } else {
        loop_pindex--;
        if (loop_pindex < 0) {
            loop_pindex = loop_paths.size() - 1;
        }
    }
}

/**
 ****************************************************************************************************************
 *  ImageAnimationPrivate
 ****************************************************************************************************************
 */

ImageAnimationPrivate::ImageAnimationPrivate(ImageAnimation *qq) :
    m_factor(0.0f), m_funval(0.0f), m_animationType(AnimationType::BlindsEffect), queue(nullptr),
    m_animationTimer(nullptr), m_staticTimer(nullptr),  q_ptr(qq)
{
    Q_UNUSED(m_padding2);
}

ImageAnimationPrivate::~ImageAnimationPrivate()
{

}

void ImageAnimationPrivate::effectPainter(QPainter *painter, const QRect &rect)
{
    if (m_pixmap1.isNull() || m_pixmap2.isNull()) {
        return;
    }
    if (m_animationTimer) {
        if (!m_animationTimer->isActive())
            return;
    }
    if (m_staticTimer) {
        if (!m_staticTimer->isActive())
            return;
    }
    centrePoint = rect.center();
//    finalX = centrePoint.x() - m_pixmap2.width() / 2;
//    finalY = centrePoint.y() - m_pixmap2.height() / 2;
//    beginX = centrePoint.x() - m_pixmap1.width() / 2;
//    beginY = centrePoint.y() - m_pixmap1.height() / 2;
//    qDebug() << finalX << finalY << beginX << beginY;
//    if (!m_animationTimer) {
//        painter->drawPixmap(finalX, finalY, m_pixmap2);
//    }
    switch (m_animationType) {
    case 0:
        fadeEffect(painter, rect, m_factor, m_pixmap1, m_pixmap2);
        break;
    case 1:
        blindsEffect(painter, rect, m_factor, m_pixmap1, m_pixmap2);
        break;
    case 2:
        flipRightToLeft(painter, rect, m_factor, m_pixmap1, m_pixmap2);
        break;
    case 3:
        outsideToInside(painter, rect, m_factor, m_pixmap1, m_pixmap2);
        break;
    case 4:
        moveLeftToRightEffect(painter, rect, m_factor, m_pixmap1, m_pixmap2);
        break;
    case 5:
        moveRightToLeftEffect(painter, rect, m_factor, m_pixmap1, m_pixmap2);
        break;
    case 6:
        moveBottomToUpEffect(painter, rect, m_factor, m_pixmap1, m_pixmap2);
        break;
    case 7:
        moveUpToBottomEffect(painter, rect, m_factor, m_pixmap1, m_pixmap2);
        break;
    case 8:
        moveBottomToLeftUpEffect(painter, rect, m_factor, m_pixmap1, m_pixmap2);
        break;
    }
    painter->end();
}

void ImageAnimationPrivate::forwardPainter(QPainter *painter, const QRect &rect)
{
    Q_UNUSED(rect);
    if (m_pixmap1.isNull() || m_pixmap2.isNull()) {
        return;
    }
    Q_Q(ImageAnimation);
    if (!m_animationTimer && !m_staticTimer) {
        setImage2(queue->jumpTonext());
        painter->drawPixmap(0, 0, m_pixmap2);
        q->setPaintTarget(ImageAnimation::KeepStatic);
        return;
    }
    if (m_animationTimer) {
        m_animationTimer->stop();
        m_animationTimer->setInterval(0);
        m_factor = 0.0f;
        painter->drawPixmap(0, 0, m_pixmap2);
        q->setPaintTarget(ImageAnimation::KeepStatic);
        m_animationTimer->deleteLater();
    }
    if (m_staticTimer) {
        if (m_animationTimer)
            if (m_animationTimer->timerId() >= 0)
                killTimer(m_animationTimer->timerId());
        stopStatic();
        if (m_staticTimer->timerId() >= 0)
            killTimer(m_staticTimer->timerId());
    }

    q->update();
}

void ImageAnimationPrivate::retreatPainter(QPainter *painter, const QRect &rect)
{
    Q_UNUSED(rect);
    if (m_pixmap1.isNull() || m_pixmap2.isNull()) {
        return;
    }
    Q_Q(ImageAnimation);
    if (!m_animationTimer && !m_staticTimer) {
        setImage2(queue->jumpTopre());
        painter->drawPixmap(0, 0, m_pixmap2);
        q->setPaintTarget(ImageAnimation::KeepStatic);
        return;
    }

    //动画播放中
    if (m_animationTimer) {
        m_animationTimer->stop();
        m_animationTimer->setInterval(0);
        m_factor = 0.0f;
        setImage2(queue->jumpTopre());
        painter->drawPixmap(0, 0, m_pixmap2);
        q->setPaintTarget(ImageAnimation::KeepStatic);
        m_animationTimer->deleteLater();
    }
    if (m_staticTimer) {
        if (m_animationTimer->timerId() >= 0)
            killTimer(m_animationTimer->timerId());
        stopStatic();
        if (m_staticTimer->timerId() >= 0)
            killTimer(m_staticTimer->timerId());
    }
}

void ImageAnimationPrivate::keepStaticPainter(QPainter *painter, const QRect &rect)
{
    Q_UNUSED(rect);
    painter->drawPixmap(0, 0, m_pixmap2);
}

void ImageAnimationPrivate::fadeEffect(QPainter *painter, const QRect &rect, float factor, const QPixmap &pixmap1, const QPixmap &pixmap2)
{
    factor = factor + FACTOR_STEP > 1.0f ? 1.0f : factor;
    int alpha = static_cast<int>(255 * (1.0f - factor));
    QPixmap alphaPixmap(rect.size());
    alphaPixmap.fill(Qt::transparent);


    QPainter p1(&alphaPixmap);
    p1.setCompositionMode(QPainter::CompositionMode_Source);
    p1.drawPixmap(0, 0, pixmap1);
    p1.setCompositionMode(QPainter::CompositionMode_DestinationIn);
    p1.fillRect(alphaPixmap.rect(), QColor(0, 0, 0, alpha));
    p1.end();

    painter->drawPixmap(0, 0, alphaPixmap);
    alpha = 255 - alpha;
    alphaPixmap.fill(Qt::transparent);
    QPainter p2(&alphaPixmap);
    p2.setCompositionMode(QPainter::CompositionMode_Source);
    p2.drawPixmap(0, 0, pixmap2);
    p2.setCompositionMode(QPainter::CompositionMode_DestinationIn);
    p2.fillRect(alphaPixmap.rect(), QColor(0, 0, 0, alpha));
    p2.end();
    painter->drawPixmap(0, 0, alphaPixmap);
}

void ImageAnimationPrivate::blindsEffect(QPainter *painter, const QRect &rect, float factor, const QPixmap &pixmap1, const QPixmap &pixmap2)
{
    Q_UNUSED(rect);
    factor = factor + FACTOR_STEP > 1.0f ? 1.0f : factor;
    int i, n, dh, ddh;
    painter->drawPixmap(0, 0, pixmap1);
    n = 10;
    dh = pixmap2.height() / n;
    ddh = static_cast<int>(factor * dh);
    if (ddh < 1) {
        ddh = 1;
    }
    for (i = 0; i < n; i++) {
        painter->drawPixmap(0, 0 + i * dh, pixmap2, 0, i * dh, pixmap2.width(), ddh);
    }
}

void ImageAnimationPrivate::flipRightToLeft(QPainter *painter, const QRect &rect, float factor, const QPixmap &pixmap1, const QPixmap &pixmap2)
{
    int w, h;
    float rot;
    QTransform trans;

    w = rect.width();
    h = rect.height();

    rot = factor * 90.0f;
    trans.translate(static_cast<qreal>(w * (1 - factor)), h / 2);
    trans.rotate(static_cast<qreal>(rot), Qt::YAxis);
    trans.translate(-w, -h / 2);

    painter->setTransform(trans);
    painter->drawPixmap(0, 0, pixmap1);
    painter->resetTransform();

    trans.reset();
    rot = 90 * (factor - 1);
    trans.translate(static_cast<qreal>(w * (1 - factor)), h / 2);
    trans.rotate(static_cast<qreal>(rot), Qt::YAxis);
    trans.translate(0, -h / 2);

    painter->setTransform(trans);
    painter->drawPixmap(0, 0, pixmap2);
    painter->resetTransform();
}

void ImageAnimationPrivate::outsideToInside(QPainter *painter, const QRect &rect, float factor, const QPixmap &pixmap1, const QPixmap &pixmap2)
{
    int   w, h, x3, y3, dh, ddh;
    w = rect.width();
    h = rect.height();
    painter->drawPixmap(0, 0, pixmap1);
    dh = pixmap2.height() / 2;
    ddh = static_cast<int>(factor * dh);
    if (ddh < 1) {
        ddh = 1;
    }
    painter->drawPixmap(0, 0, pixmap2, 0, 0, pixmap2.width(), ddh);
    x3 = (w - pixmap2.width()) / 2;
    y3 = static_cast<int>(dh * (1.0f - factor) + h / 2);
    if (y3 != h / 2)
        y3 += 1;
    painter->drawPixmap(x3, y3, pixmap2, 0, pixmap2.height() - ddh, pixmap2.width(), ddh);
}

void ImageAnimationPrivate::moveLeftToRightEffect(QPainter *painter, const QRect &rect, float factor, const QPixmap &pixmap1, const QPixmap &pixmap2)
{
    int x, y, w, h;
    w = rect.width();
    h = rect.height();
    x = static_cast<int>(0 + w * factor);
    y = 0;
    painter->drawPixmap(x, y, pixmap1);
    x = static_cast<int>(0 + w * (factor - 1));
    y = 0;
    painter->drawPixmap(x, y, pixmap2);
}

void ImageAnimationPrivate::moveRightToLeftEffect(QPainter *painter, const QRect &rect, float factor, const QPixmap &pixmap1, const QPixmap &pixmap2)
{

}

void ImageAnimationPrivate::moveBottomToUpEffect(QPainter *painter, const QRect &rect, float factor, const QPixmap &pixmap1, const QPixmap &pixmap2)
{

}

void ImageAnimationPrivate::moveUpToBottomEffect(QPainter *painter, const QRect &rect, float factor, const QPixmap &pixmap1, const QPixmap &pixmap2)
{

}

void ImageAnimationPrivate::moveBottomToLeftUpEffect(QPainter *painter, const QRect &rect, float factor, const QPixmap &pixmap1, const QPixmap &pixmap2)
{

}

void ImageAnimationPrivate::setPathList(const QString &first, const QStringList &list)
{
    queue = QSharedPointer<LoopQueue>(new LoopQueue(first, list));
    setImage1(queue->first());
    setImage2(queue->second());
}

void ImageAnimationPrivate::startAnimation()
{
    Q_Q(ImageAnimation);
    if (!m_animationTimer) {
        m_animationTimer = new QTimer();
        connect(m_animationTimer, &QTimer::timeout, this, [ = ]() {
            m_funval += FACTOR_STEP;
            m_factor += GaussFunction(0.05, 0.5f, 5, m_funval);
            if (m_factor + 0.005f > 1.0f)
                m_factor = 1.0f;
            if (m_funval > 1.0f) {
                m_funval = 0.0f;
                m_factor = 0.0f;
                stopAnimation();
                startStatic();
            } else {
                qDebug() << "animation running";
                m_animationTimer->start(UPDATE_RATE);
                q->update();
            }
        });
    }
    m_animationTimer->start(UPDATE_RATE);
}

void ImageAnimationPrivate::stopAnimation()
{
    if (m_animationTimer)
        m_animationTimer->stop();
    delete m_animationTimer;
}

void ImageAnimationPrivate::startStatic()
{
    if (!m_staticTimer) {
        m_staticTimer = new QTimer;
        setImage1(m_imageName2);
        setImage2(queue->jumpTonext());
        connect(m_staticTimer, &QTimer::timeout, this, [ = ]() {
            qDebug() << "static stop";
            stopStatic();
            qsrand(static_cast<uint>(QTime(0, 0, 0).secsTo(QTime::currentTime())));
            m_animationType = static_cast<AnimationType>(qrand() % (3));
            startAnimation();
        });
        qDebug() << "static begin " << queue->index();
    }
    m_staticTimer->start(2500);
}

void ImageAnimationPrivate::stopStatic()
{
    if (m_staticTimer)
        m_staticTimer->stop();
    delete m_staticTimer;
}

void ImageAnimationPrivate::setImage1(const QString &imageName1_bar)
{
    m_imageName1 = imageName1_bar;
    QImage tImg;
    QString errMsg;
    UnionImage_NameSpace::loadStaticImageFromFile(imageName1_bar, tImg, errMsg);
    QPixmap p1 = QPixmap::fromImage(tImg);
    int beginX = 0, beginY = 0;
    if (p1.width() > p1.height()) {
        m_pixmap1 = QPixmap(dApp->desktop()->screenGeometry().size());
        QPainter pa1(&m_pixmap1);
        m_pixmap1.fill(QColor("#252525"));
        p1 = p1.scaledToWidth(dApp->desktop()->screenGeometry().width());
        centrePoint = dApp->desktop()->screenGeometry().center();
        beginX = centrePoint.x() - p1.width() / 2;
        beginX = beginX > 0 ? beginX : 0;
        beginY = centrePoint.y() - p1.height() / 2;
        beginY = beginY > 0 ? beginY : 0;
        pa1.drawPixmap(beginX, beginY, p1);
        pa1.end();
    } else {
        m_pixmap1 = QPixmap(dApp->desktop()->screenGeometry().size());
        QPainter pa1(&m_pixmap1);
        m_pixmap1.fill(QColor("#252525"));
        p1 = p1.scaledToHeight(dApp->desktop()->screenGeometry().height() + 8);
        centrePoint = dApp->desktop()->screenGeometry().center();
        beginX = centrePoint.x() - p1.width() / 2;
        beginX = beginX > 0 ? beginX : 0;
        beginY = centrePoint.y() - p1.height() / 2;
        beginY = beginY > 0 ? beginY : 0;
        pa1.drawPixmap(beginX, beginY, p1);
        pa1.end();
    }
}

void ImageAnimationPrivate::setImage2(const QString &imageName2_bar)
{
    m_imageName2 = imageName2_bar;
    int beginX = 0, beginY = 0;
    QImage tImg;
    QString errMsg;
    UnionImage_NameSpace::loadStaticImageFromFile(imageName2_bar, tImg, errMsg);
    QPixmap p2 = QPixmap::fromImage(tImg);
    if (p2.width() > p2.height()) {
        m_pixmap2 = QPixmap(dApp->desktop()->screenGeometry().size());
        QPainter pa2(&m_pixmap2);
        m_pixmap2.fill(QColor("#252525"));
        p2 = p2.scaledToWidth(dApp->desktop()->screenGeometry().width());
        centrePoint = dApp->desktop()->screenGeometry().center();
        beginX = centrePoint.x() - p2.width() / 2;
        beginX = beginX > 0 ? beginX : 0;
        beginY = centrePoint.y() - p2.height() / 2;
        beginY = beginY > 0 ? beginY : 0;
        pa2.drawPixmap(beginX, beginY, p2);
        pa2.end();
    } else {
        m_pixmap2 = QPixmap(dApp->desktop()->screenGeometry().size());
        QPainter pa2(&m_pixmap2);
        m_pixmap2.fill(QColor("#252525"));
        p2 = p2.scaledToHeight(dApp->desktop()->screenGeometry().height() + 8);
        centrePoint = dApp->desktop()->screenGeometry().center();
        beginX = centrePoint.x() - p2.width() / 2;
        beginX = beginX > 0 ? beginX : 0;
        beginY = centrePoint.y() - p2.height() / 2;
        beginY = beginY > 0 ? beginY : 0;
        pa2.drawPixmap(beginX, beginY, p2);
        pa2.end();
    }
}

/**
 ****************************************************************************************************************
 *  ImageAnimation
 ****************************************************************************************************************
 */
ImageAnimation::ImageAnimation(QWidget *parent) :
    QWidget(parent), current_target(EffectPlay), d_ptr(new ImageAnimationPrivate(this))
{
    setAttribute(Qt::WA_TransparentForMouseEvents, true);
    setAttribute(Qt::WA_StyledBackground, true);
    setStyleSheet("background-color:#252525");
}

ImageAnimation::~ImageAnimation()
{
    Q_D(ImageAnimation);
    delete d;
}

void ImageAnimation::startSlideShow(const QString &beginPath, const QStringList &pathlist)
{
    Q_D(ImageAnimation);
    setPaintTarget(EffectPlay);
    d->setPathList(beginPath, pathlist);
    d->startAnimation();
}
void ImageAnimation::stopSlideShow()
{
    Q_D(ImageAnimation);
    d->stopAnimation();
    d->stopStatic();
}

void ImageAnimation::pauseAndnext()
{
    setPaintTarget(SkipToNext);
    update();
}

void ImageAnimation::ifPauseAndContinue()
{
    Q_D(ImageAnimation);
    if (current_target != EffectPlay) {
        setPaintTarget(EffectPlay);
        d->startAnimation();
    }
}

void ImageAnimation::pauseAndpre()
{
    setPaintTarget(TurnBackPre);
    update();
}

void ImageAnimation::paintEvent(QPaintEvent *e)
{
    QWidget::paintEvent(e);
    Q_D(ImageAnimation);
    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing, true);
    switch (current_target) {
    case EffectPlay: {
        d->effectPainter(&painter, dApp->desktop()->screenGeometry());
        break;
    }
    case SkipToNext: {
        d->forwardPainter(&painter, dApp->desktop()->screenGeometry());
        break;
    }
    case TurnBackPre: {
        d->retreatPainter(&painter, dApp->desktop()->screenGeometry());
        break;
    }
    case KeepStatic: {
        d->keepStaticPainter(&painter, dApp->desktop()->screenGeometry());
        break;
    }
    }
}

void ImageAnimation::setPaintTarget(PaintTarget target)
{
    current_target = target;
}
