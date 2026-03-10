#ifndef ACRYLICHELPER_H
#define ACRYLICHELPER_H

#include <QObject>
#include <QColor>
#include <QAbstractNativeEventFilter>

class QWidget;

// Windows acrylic blur helper for Qt5 widgets.
// API inspired by Win32-Acrylic-Effect's AcrylicCompositor.
class AcrylicHelper : public QObject, public QAbstractNativeEventFilter
{
    Q_OBJECT

public:
    // Mirrors AcrylicCompositor::BackdropSource
    enum BackdropSource {
        DesktopBackdrop = 0,
        HostBackdrop = 1
    };

    // Mirrors AcrylicCompositor::AcrylicEffectParameter
    struct EffectParams {
        float blurAmount = 20.0f;
        float saturationAmount = 1.25f;
        QColor tintColor = QColor(32, 32, 32, 166);    // acrelic-dark.html: rgba(32,32,32,0.65)
        QColor fallbackColor = QColor(32, 32, 32, 255);
    };

    explicit AcrylicHelper(QWidget* targetWidget, QObject* parent = nullptr);
    ~AcrylicHelper() override;

    // Main API - mirrors AcrylicCompositor::SetAcrylicEffect
    bool setAcrylicEffect(BackdropSource source, const EffectParams& params);

    // Update the pill-shaped blur region (call after resize)
    void updateBlurRegion();

    // QAbstractNativeEventFilter - mirrors AcrylicCompositor::Sync
    bool nativeEventFilter(const QByteArray& eventType, void* message, long* result) override;

private:
    long getBuildVersion();
    bool applyBlurBehind();
    bool applyAcrylicAccent();
    void enableDarkMode();
    void syncOnActivate(bool active);

    QWidget* m_widget;
    quintptr m_hwnd = 0;  // Cached HWND - never call winId() outside setAcrylicEffect()
    EffectParams m_params;
    BackdropSource m_source = DesktopBackdrop;
    bool m_active = true;
    bool m_effectApplied = false;
};

#endif // ACRYLICHELPER_H
