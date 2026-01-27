#ifndef TOPBAR_H
#define TOPBAR_H

#include <QWidget>
#include <QPushButton>
#include <QHBoxLayout>
#include <QLabel>
#include <QFrame>
#include <QPoint>

// Icon types for tool buttons
enum class IconType {
    None,
    Monitor,
    Keyboard,
    Settings,
    Power
};

class ToolButton : public QWidget {
    Q_OBJECT

public:
    explicit ToolButton(IconType icon, const QString& tooltip, QWidget* parent = nullptr);
    explicit ToolButton(const QString& text, const QString& tooltip, bool isLangSwitcher, QWidget* parent = nullptr);

    void setIconColor(const QColor& color);
    bool isLangSwitcher() const { return m_isLangSwitcher; }

signals:
    void clicked();
    void dragStarted(const QPoint& globalPos);
    void dragging(const QPoint& globalPos);
    void dragFinished();

protected:
    void paintEvent(QPaintEvent* event) override;
    void enterEvent(QEvent* event) override;
    void leaveEvent(QEvent* event) override;
    void mousePressEvent(QMouseEvent* event) override;
    void mouseMoveEvent(QMouseEvent* event) override;
    void mouseReleaseEvent(QMouseEvent* event) override;

private:
    void drawIcon(QPainter& painter, IconType icon);

    IconType m_iconType = IconType::None;
    QString m_text;
    QString m_tooltip;
    bool m_isLangSwitcher = false;
    bool m_hovered = false;
    bool m_pressed = false;
    bool m_isDragging = false;
    QPoint m_dragStartPos;
    QColor m_iconColor;
};

class TopBar : public QWidget {
    Q_OBJECT

public:
    explicit TopBar(QWidget* parent = nullptr);
    ~TopBar() override;

    void enableBlur();

protected:
    void paintEvent(QPaintEvent* event) override;
    void resizeEvent(QResizeEvent* event) override;

signals:
    void languageSwitcherClicked();
    void monitorSettingsClicked();
    void inputSettingsClicked();
    void settingsClicked();
    void powerClicked();

private slots:
    void onDragStarted(const QPoint& globalPos);
    void onDragging(const QPoint& globalPos);
    void onDragFinished();

private:
    QFrame* createSeparator();
    void setupUI();
    void applyPlatformBlur();
    void updateMask();

    QHBoxLayout* m_layout;
    ToolButton* m_langSwitcher;
    ToolButton* m_monitorBtn;
    ToolButton* m_keyboardBtn;
    ToolButton* m_settingsBtn;
    ToolButton* m_powerBtn;

    // Drag state
    bool m_isDragging = false;
    QPoint m_dragOffset;

    // Platform blur state
    bool m_blurEnabled = false;
};

#endif // TOPBAR_H
