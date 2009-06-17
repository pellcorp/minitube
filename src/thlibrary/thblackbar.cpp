#include <QPaintEvent>
#include <QList>

#include "thblackbutton.h"
#include "thblackbar.h"
#include "thpainter.h"
#include "thaction.h"

/* ============================================================================
 *  PRIVATE Class
 */
class THBlackBar::Private {
        public:
    QList<THAction *> actionList;
    THAction *checkedAction;
    THAction *hoveredAction;
};

/* ============================================================================
 *  PUBLIC Constructor/Destructors
 */
THBlackBar::THBlackBar (QWidget *parent)
    : QWidget(parent), d(new THBlackBar::Private)
{
    // Setup Widget Options
    setMouseTracking(true);

    // Setup Members
    d->hoveredAction = NULL;
    d->checkedAction = NULL;
}

THBlackBar::~THBlackBar() {
    delete d;
}

/* ============================================================================
 *  PUBLIC Methods
 */
THAction *THBlackBar::addAction (THAction *action) {
    d->actionList.append(action);
    return(action);
}

THAction *THBlackBar::addAction (const QString& text) {
    THAction *action = new THAction(text, this);
    d->actionList.append(action);
    return(action);
}

void THBlackBar::setCheckedAction(int index) {
    if (d->checkedAction)
        d->checkedAction->setChecked(false);
    d->checkedAction = d->actionList.at(index);
    d->checkedAction->setChecked(true);
    update();
}

QSize THBlackBar::minimumSizeHint (void) const {	
    int itemsWidth = calculateButtonWidth() * d->actionList.size();
    return(QSize(100 + itemsWidth, 32));
}

/* ============================================================================
 *  PROTECTED Methods
 */
void THBlackBar::paintEvent (QPaintEvent *event) {
    int height = event->rect().height();
    int width = event->rect().width();
    // int mh = (height / 2);

    // THPainter p(this);
    QPainter p(this);

    /*
    // Draw Background
    QLinearGradient linearGradUp(QPointF(0, 0), QPointF(0, mh));
    linearGradUp.setColorAt(0, QColor(0x97, 0x97, 0x97));
    linearGradUp.setColorAt(1, QColor(0x4d, 0x4d, 0x4d));
    p.fillRect(0, 0, width, mh, QBrush(linearGradUp));

    QLinearGradient linearGradDw(QPointF(0, mh), QPointF(0, height));
    linearGradDw.setColorAt(0, QColor(0x3a, 0x3a, 0x3a));
    linearGradDw.setColorAt(1, QColor(0x42, 0x42, 0x42));
    p.fillRect(0, mh, width, mh, QBrush(linearGradDw));
    */
    
    // Calculate Buttons Size & Location
    int buttonWidth = width / d->actionList.size(); // calculateButtonWidth();
    // int buttonsWidth = width; // buttonWidth * d->actionList.size();
    int buttonsX = 0; // (width / 2) - (buttonsWidth / 2);

    // Draw Buttons
    // p.translate(0, 4);
    QRect rect(buttonsX, 0, buttonWidth, height);
    foreach (THAction *action, d->actionList) {
        drawButton(&p, rect, action);
        rect.moveLeft(rect.x() + rect.width());
    }
    // p.translate(0, -4);

    // Draw Buttons Shadow
    // p.fillRect(buttonsX, height - 4, buttonsWidth, 1, QColor(0x6d, 0x6d, 0x6d));

    p.end();
}

void THBlackBar::mouseMoveEvent (QMouseEvent *event) {
    QWidget::mouseMoveEvent(event);

    THAction *action = hoveredAction(event->pos());
    if (action == NULL && d->hoveredAction != NULL) {
        d->hoveredAction->hover(false);
        d->hoveredAction = NULL;
        update();
    } else if (action != NULL) {
        d->hoveredAction = action;
        action->hover(true);
        update();
    }
}

void THBlackBar::mousePressEvent (QMouseEvent *event) {
    QWidget::mousePressEvent(event);

    if (d->hoveredAction != NULL) {

        if (d->checkedAction != NULL) {
            // already checked
            if (d->checkedAction == d->hoveredAction) return;
            d->checkedAction->setChecked(false);
        }

        d->checkedAction = d->hoveredAction;
        d->hoveredAction->setChecked(true);
        d->hoveredAction->trigger();

        update();
    }
}

THAction *THBlackBar::hoveredAction (const QPoint& pos) const {
    if (pos.y() <= 0 || pos.y() >= height())
        return(NULL);

    /*
    int buttonWidth = calculateButtonWidth();
    int buttonsWidth = buttonWidth * d->actionList.size();
    int buttonsX = (width() / 2) - (buttonsWidth / 2);
    */
    
    int buttonWidth = width() / d->actionList.size(); // calculateButtonWidth();
    int buttonsWidth = width(); // buttonWidth * d->actionList.size();
    int buttonsX = 0; // (width / 2) - (buttonsWidth / 2);
    
    if (pos.x() <= buttonsX || pos.x() >= (buttonsX + buttonsWidth))
        return(NULL);

    int buttonIndex = (pos.x() - buttonsX) / buttonWidth;

    if (buttonIndex >= d->actionList.size())
        return(NULL);
    return(d->actionList[buttonIndex]);
}

int THBlackBar::calculateButtonWidth (void) const {
    QFont smallerBoldFont;
    smallerBoldFont.setBold(true);
    smallerBoldFont.setPointSize(smallerBoldFont.pointSize()*.85);
    QFontMetrics fontMetrics(smallerBoldFont);
    int tmpItemWidth, itemWidth = 0;
    foreach (THAction *action, d->actionList) {
        tmpItemWidth = fontMetrics.width(action->text());
        if (itemWidth < tmpItemWidth) itemWidth = tmpItemWidth;
    }
    return itemWidth;
}


/* ============================================================================
 *  PRIVATE Methods
 */
void THBlackBar::drawUnselectedButton (	QPainter *painter,
                                        const QRect& rect,
                                        const THAction *action)
{
    QLinearGradient linearGrad(QPointF(0, 0), QPointF(0, rect.height() / 2));    
    linearGrad.setColorAt(0, QColor(0x8e, 0x8e, 0x8e));
    linearGrad.setColorAt(1, QColor(0x5c, 0x5c, 0x5c));
    /*
    QPalette palette;
    linearGrad.setColorAt(0, palette.color(QPalette::Dark));
    linearGrad.setColorAt(1, palette.color(QPalette::Midlight));
*/
    drawButton(painter, rect, linearGrad, QColor(0x41, 0x41, 0x41), action);
    // drawButton(painter, rect, linearGrad, palette.color(QPalette::Shadow), action);
}

void THBlackBar::drawSelectedButton (	QPainter *painter,
                                        const QRect& rect,
                                        const THAction *action)
{
    QLinearGradient linearGrad(QPointF(0, 0), QPointF(0, rect.height() / 2));
    linearGrad.setColorAt(0, QColor(0x6d, 0x6d, 0x6d));
    linearGrad.setColorAt(1, QColor(0x25, 0x25, 0x25));
    drawButton(painter, rect, linearGrad, QColor(0x00, 0x00, 0x00), action);
}

void THBlackBar::drawButton (	QPainter *painter,
                                const QRect& rect,
                                const THAction *action)
{
    if (action->isChecked())
        drawSelectedButton(painter, rect, action);
    else
        drawUnselectedButton(painter, rect, action);
}

void THBlackBar::drawButton (	QPainter *painter, 
                                const QRect& rect,
                                const QLinearGradient& gradient,
                                const QColor& color,
                                const THAction *action)
{
    painter->save();

    int height = rect.height();
    int width = rect.width();
    int mh = (height / 2);

    painter->translate(rect.x(), rect.y());
    painter->setPen(QColor(0x28, 0x28, 0x28));

    painter->fillRect(0, 0, width, mh, QBrush(gradient));
    painter->fillRect(0, mh, width, mh, color);
    painter->drawRect(0, 0, width, height);

    QFont smallerBoldFont;
    smallerBoldFont.setBold(true);
    smallerBoldFont.setPointSize(smallerBoldFont.pointSize()*.85);
    painter->setFont(smallerBoldFont);
    painter->setPen(QPen(QColor(0xff, 0xff, 0xff), 1));
    painter->drawText(0, 1, width, height, Qt::AlignCenter, action->text());

    painter->restore();
}

