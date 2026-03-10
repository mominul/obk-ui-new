#include "TopBar.h"
#include "AcrylicHelper.h"

#include <QPainter>
#include <QPainterPath>
#include <QPalette>
#include <QGraphicsDropShadowEffect>
#include <QMouseEvent>
#include <QApplication>
#include <QProcess>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

#ifdef Q_OS_MAC
extern "C" {
    void enableMacOSBlur(void* nsView);
}
#endif

// Design tokens from HTML
namespace FluentDesign {
    const QColor AcrylicTintDark(32, 32, 32, 166);      // rgba(32, 32, 32, 0.65)
    const QColor BorderSubtle(255, 255, 255, 20);       // rgba(255, 255, 255, 0.08)
    const QColor BorderStrong(255, 255, 255, 38);       // rgba(255, 255, 255, 0.15)
    const QColor AccentRed("#d13438");
    const QColor TextPrimary(255, 255, 255);
    const QColor HoverBackground(255, 255, 255, 26);    // rgba(255, 255, 255, 0.1)
    const QColor ActiveBackground(255, 255, 255, 13);   // rgba(255, 255, 255, 0.05)
    const QColor SeparatorColor(255, 255, 255, 26);     // rgba(255, 255, 255, 0.1)
    const QColor PowerRed("#ff4d4d");

    const int ToolbarPadding = 6;
    const int ButtonSize = 40;
    const int LangButtonSize = 36;
    const int IconSize = 20;
    const int SeparatorHeight = 24;
    const int BorderRadius = 100;  // Pill shape
    const int DragThreshold = 5;   // Pixels before drag starts
}

// ============== ToolButton Implementation ==============

ToolButton::ToolButton(IconType icon, const QString& tooltip, QWidget* parent)
    : QWidget(parent), m_iconType(icon), m_tooltip(tooltip), m_iconColor(FluentDesign::TextPrimary)
{
    setFixedSize(FluentDesign::ButtonSize, FluentDesign::ButtonSize);
    setToolTip(tooltip);
    setCursor(Qt::PointingHandCursor);
    setAttribute(Qt::WA_Hover);
}

ToolButton::ToolButton(const QString& text, const QString& tooltip, bool isLangSwitcher, QWidget* parent)
    : QWidget(parent), m_text(text), m_tooltip(tooltip), m_isLangSwitcher(isLangSwitcher),
      m_iconColor(FluentDesign::TextPrimary)
{
    if (isLangSwitcher) {
        setFixedSize(FluentDesign::LangButtonSize, FluentDesign::LangButtonSize);
        setCursor(Qt::SizeAllCursor);  // Show move cursor for drag handle
    } else {
        setFixedSize(FluentDesign::ButtonSize, FluentDesign::ButtonSize);
        setCursor(Qt::PointingHandCursor);
    }
    setToolTip(tooltip);
    setAttribute(Qt::WA_Hover);
}

void ToolButton::setIconColor(const QColor& color)
{
    m_iconColor = color;
    update();
}

void ToolButton::paintEvent(QPaintEvent* event)
{
    Q_UNUSED(event);

    QPainter painter(this);
    if (!painter.isActive()) {
        return;
    }
    painter.setRenderHint(QPainter::Antialiasing);

    QRectF buttonRect = rect();

    // Scale down when pressed
    if (m_pressed && !m_isDragging) {
        painter.translate(buttonRect.center());
        painter.scale(0.92, 0.92);
        painter.translate(-buttonRect.center());
    }

    // Draw background
    if (m_isLangSwitcher) {
        // Red background for language switcher
        QColor bgColor = FluentDesign::AccentRed;
        if (m_pressed) {
            bgColor = bgColor.darker(110);
        } else if (m_hovered) {
            bgColor = bgColor.lighter(110);
        }

        painter.fillRect(buttonRect, bgColor);

        // Draw Bengali character "অ"
        painter.setPen(Qt::white);
        QFont font;
        font.setPointSize(14);
        font.setBold(true);
        painter.setFont(font);
        painter.drawText(buttonRect, Qt::AlignCenter, m_text);

    } else {
        // Always draw a subtle background so buttons are visible
        QColor bgColor = FluentDesign::HoverBackground;
        if (m_pressed) {
            bgColor = FluentDesign::ActiveBackground;
        } else if (!m_hovered) {
            bgColor = QColor(255, 255, 255, 15); // Very subtle when not hovered
        }
        painter.fillRect(buttonRect.adjusted(2, 2, -2, -2), bgColor);

        // Draw icon
        if (m_iconType != IconType::None) {
            drawIcon(painter, m_iconType);
        } else if (!m_text.isEmpty()) {
            painter.setPen(m_iconColor);
            QFont font("Segoe UI", 14);
            painter.setFont(font);
            painter.drawText(buttonRect, Qt::AlignCenter, m_text);
        }
    }
}

void ToolButton::drawIcon(QPainter& painter, IconType icon)
{
    QRectF r = rect();
    qreal cx = r.center().x();
    qreal cy = r.center().y();
    qreal s = 8;  // Half icon size

    // Set up pen for icon drawing
    painter.setBrush(Qt::NoBrush);
    painter.setPen(QPen(m_iconColor, 2.0, Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin));

    switch (icon) {
    case IconType::Monitor: {
        // Monitor icon: rectangle with stand
        QRectF screen(cx - s, cy - s + 1, s * 2, s * 1.3);
        painter.drawRoundedRect(screen, 2, 2);
        // Stand
        painter.drawLine(QPointF(cx, screen.bottom()), QPointF(cx, cy + s - 1));
        painter.drawLine(QPointF(cx - s * 0.5, cy + s - 1), QPointF(cx + s * 0.5, cy + s - 1));
        break;
    }
    case IconType::Keyboard: {
        // Keyboard icon: rectangle with keys
        QRectF kbd(cx - s, cy - s * 0.5, s * 2, s * 1.1);
        painter.drawRoundedRect(kbd, 3, 3);
        // Keys as small rectangles
        painter.setPen(QPen(m_iconColor, 1.5, Qt::SolidLine, Qt::RoundCap));
        qreal y1 = cy - s * 0.1;
        for (int i = 0; i < 3; ++i) {
            qreal x = cx - s * 0.5 + i * s * 0.5;
            painter.drawLine(QPointF(x - 2, y1), QPointF(x + 2, y1));
        }
        // Space bar
        painter.setPen(QPen(m_iconColor, 2.0, Qt::SolidLine, Qt::RoundCap));
        qreal y2 = cy + s * 0.3;
        painter.drawLine(QPointF(cx - s * 0.5, y2), QPointF(cx + s * 0.5, y2));
        break;
    }
    case IconType::Settings: {
        // Gear icon - simplified
        qreal outerR = s * 0.85;
        qreal midR = s * 0.65;
        int teeth = 8;

        QPainterPath gear;
        for (int i = 0; i < teeth * 2; ++i) {
            qreal angle = (i * M_PI) / teeth - M_PI / 2;
            qreal radius = (i % 2 == 0) ? outerR : midR;
            qreal x = cx + radius * cos(angle);
            qreal y = cy + radius * sin(angle);
            if (i == 0) {
                gear.moveTo(x, y);
            } else {
                gear.lineTo(x, y);
            }
        }
        gear.closeSubpath();
        painter.drawPath(gear);
        // Center hole
        painter.drawEllipse(QPointF(cx, cy), s * 0.3, s * 0.3);
        break;
    }
    case IconType::Power: {
        // Power icon: arc with vertical line
        QRectF arcRect(cx - s * 0.7, cy - s * 0.4, s * 1.4, s * 1.4);
        painter.drawArc(arcRect, 60 * 16, 240 * 16);
        painter.drawLine(QPointF(cx, cy - s * 0.7), QPointF(cx, cy + s * 0.1));
        break;
    }
    default:
        break;
    }
}

void ToolButton::enterEvent(QEvent* event)
{
    m_hovered = true;
    update();
    QWidget::enterEvent(event);
}

void ToolButton::leaveEvent(QEvent* event)
{
    m_hovered = false;
    update();
    QWidget::leaveEvent(event);
}

void ToolButton::mousePressEvent(QMouseEvent* event)
{
    if (event->button() == Qt::LeftButton) {
        m_pressed = true;
        m_dragStartPos = event->globalPos();
        m_isDragging = false;
        update();
    }
    QWidget::mousePressEvent(event);
}

void ToolButton::mouseMoveEvent(QMouseEvent* event)
{
    if (m_pressed && m_isLangSwitcher) {
        QPoint delta = event->globalPos() - m_dragStartPos;
        if (!m_isDragging && delta.manhattanLength() > FluentDesign::DragThreshold) {
            m_isDragging = true;
            emit dragStarted(m_dragStartPos);
        }
        if (m_isDragging) {
            emit dragging(event->globalPos());
        }
    }
    QWidget::mouseMoveEvent(event);
}

void ToolButton::mouseReleaseEvent(QMouseEvent* event)
{
    if (event->button() == Qt::LeftButton) {
        if (m_isDragging) {
            emit dragFinished();
        } else if (m_pressed) {
            emit clicked();
        }
        m_pressed = false;
        m_isDragging = false;
        update();
    }
    QWidget::mouseReleaseEvent(event);
}

// ============== TopBar Implementation ==============

TopBar::TopBar(QWidget* parent)
    : QWidget(parent)
{
    setAttribute(Qt::WA_TranslucentBackground);
    setWindowFlags(Qt::FramelessWindowHint | Qt::Tool | Qt::WindowStaysOnTopHint);

    setupUI();
}

TopBar::~TopBar() = default;

void TopBar::setupUI()
{
    m_layout = new QHBoxLayout(this);
    m_layout->setContentsMargins(12, 6, 12, 6);
    m_layout->setSpacing(8);

    // Language Switcher (drag handle)
    m_langSwitcher = new ToolButton(QString::fromUtf8("অ"), "IME Switcher (Drag to move)", true, this);
    connect(m_langSwitcher, &ToolButton::clicked, this, &TopBar::languageSwitcherClicked);
    connect(m_langSwitcher, &ToolButton::dragStarted, this, &TopBar::onDragStarted);
    connect(m_langSwitcher, &ToolButton::dragging, this, &TopBar::onDragging);
    connect(m_langSwitcher, &ToolButton::dragFinished, this, &TopBar::onDragFinished);
    m_layout->addWidget(m_langSwitcher);

    // First separator
    m_layout->addWidget(createSeparator());

    // Monitor Settings
    m_monitorBtn = new ToolButton(IconType::Monitor, "Monitor Settings", this);
    connect(m_monitorBtn, &ToolButton::clicked, this, &TopBar::monitorSettingsClicked);
    m_layout->addWidget(m_monitorBtn);

    // Keyboard Settings
    m_keyboardBtn = new ToolButton(IconType::Keyboard, "Input Settings", this);
    connect(m_keyboardBtn, &ToolButton::clicked, this, &TopBar::inputSettingsClicked);
    m_layout->addWidget(m_keyboardBtn);

    // Settings
    m_settingsBtn = new ToolButton(IconType::Settings, "Settings", this);
    connect(m_settingsBtn, &ToolButton::clicked, this, &TopBar::settingsClicked);
    m_layout->addWidget(m_settingsBtn);

    // Second separator
    m_layout->addWidget(createSeparator());

    // Power Button
    m_powerBtn = new ToolButton(IconType::Power, "Power Options", this);
    m_powerBtn->setIconColor(FluentDesign::PowerRed);
    connect(m_powerBtn, &ToolButton::clicked, this, &TopBar::powerClicked);
    m_layout->addWidget(m_powerBtn);

    setLayout(m_layout);
    adjustSize();
}

QFrame* TopBar::createSeparator()
{
    auto* separator = new QFrame(this);
    separator->setFrameShape(QFrame::NoFrame);
    separator->setFixedSize(1, FluentDesign::SeparatorHeight);
    separator->setAutoFillBackground(true);
    QPalette pal = separator->palette();
    pal.setColor(QPalette::Window, QColor(255, 255, 255, 40));
    separator->setPalette(pal);
    return separator;
}

void TopBar::onDragStarted(const QPoint& globalPos)
{
    m_isDragging = true;
    m_dragOffset = globalPos - frameGeometry().topLeft();
}

void TopBar::onDragging(const QPoint& globalPos)
{
    if (m_isDragging) {
        move(globalPos - m_dragOffset);
    }
}

void TopBar::onDragFinished()
{
    m_isDragging = false;
}

void TopBar::paintEvent(QPaintEvent* event)
{
    Q_UNUSED(event);

    QPainter painter(this);
    if (!painter.isActive()) return;

    painter.setRenderHint(QPainter::Antialiasing);

    QPainterPath path;
    QRectF r = QRectF(rect()).adjusted(0.5, 0.5, -0.5, -0.5);
    qreal radius = r.height() / 2.0;
    path.addRoundedRect(r, radius, radius);

    if (m_blurActive) {
        // Acrylic mode: fill the pill area with a minimal-alpha color.
        // The accent policy's GradientColor provides the dark tint over the blur.
        // We just need alpha > 0 so the pill area is "visible" to DWM and the
        // acrylic renders here. Corners stay alpha=0 (transparent/click-through).
        painter.fillPath(path, QColor(0, 0, 0, 1));
    } else {
        // Fallback: solid dark tint (no blur available)
        painter.fillPath(path, FluentDesign::AcrylicTintDark);
    }

    // Draw subtle border
    painter.setPen(QPen(FluentDesign::BorderStrong, 1));
    painter.drawPath(path);
}

void TopBar::resizeEvent(QResizeEvent* event)
{
    QWidget::resizeEvent(event);
}

void TopBar::enableBlur()
{
#ifdef Q_OS_WIN
    // Use AcrylicHelper (API inspired by Win32-Acrylic-Effect's AcrylicCompositor)
    m_acrylicHelper = new AcrylicHelper(this, this);

    AcrylicHelper::EffectParams params;
    // Design tokens from acrelic-dark.html
    params.blurAmount      = 20.0f;                        // backdrop-filter: blur(20px)
    params.saturationAmount = 1.25f;                       // saturate(125%)
    params.tintColor       = FluentDesign::AcrylicTintDark; // rgba(32, 32, 32, 0.65)
    params.fallbackColor   = QColor(32, 32, 32, 255);

    m_blurActive = m_acrylicHelper->setAcrylicEffect(AcrylicHelper::HostBackdrop, params);

#elif defined(Q_OS_MAC)
    enableMacOSBlur(reinterpret_cast<void*>(winId()));

#elif defined(Q_OS_LINUX)
    WId wid = winId();
    QString command = QString("xprop -f _KDE_NET_WM_BLUR_BEHIND_REGION 32c "
                              "-set _KDE_NET_WM_BLUR_BEHIND_REGION 0 -id %1").arg(wid);
    QProcess::startDetached("sh", QStringList() << "-c" << command);
#endif
}
