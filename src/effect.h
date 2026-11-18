#pragma once

#include <optional>

#include <kwin/effect/effect.h>

#include <QHash>
#include <QStringList>

namespace KWin {
class Window;

class SecureCaptureEffect final : public Effect {
  Q_OBJECT
  Q_CLASSINFO("D-Bus Interface", "org.kde.SecureCapture")
  Q_PROPERTY(bool active READ active)
  Q_PROPERTY(bool manualEnabled READ manualEnabled)
  Q_PROPERTY(bool manualDisabled READ manualDisabled)
  Q_PROPERTY(bool protectDesktop READ protectDesktop)
  Q_PROPERTY(bool protectPanels READ protectPanels)
  Q_PROPERTY(bool protectTabSwitcher READ protectTabSwitcher)
  Q_PROPERTY(bool backendAvailable READ backendAvailable)
  Q_PROPERTY(QStringList allowlist READ allowlist)
  Q_PROPERTY(QStringList triggers READ triggers)

public:
  explicit SecureCaptureEffect();
  ~SecureCaptureEffect() override;

  bool active() const;
  bool manualEnabled() const;
  bool manualDisabled() const;
  bool protectDesktop() const;
  bool protectPanels() const;
  bool protectTabSwitcher() const;
  bool backendAvailable() const;
  QStringList allowlist() const;
  QStringList triggers() const;

public Q_SLOTS:
  Q_SCRIPTABLE void setEnabled(bool enabled);
  Q_SCRIPTABLE void setDisabled(bool disabled);
  Q_SCRIPTABLE void setProtectDesktop(bool protect);
  Q_SCRIPTABLE void setProtectPanels(bool protect);
  Q_SCRIPTABLE void setProtectTabSwitcher(bool protect);
  Q_SCRIPTABLE void setAllowlist(const QStringList &patterns);
  Q_SCRIPTABLE void setTriggers(const QStringList &rules);

private:
  struct Trigger {
    QString pattern;
    bool enable = false;
  };

  void loadConfiguration();
  void saveConfiguration();
  void updateState();
  void applyToAllWindows();
  void applyToWindow(EffectWindow *window);
  void restoreModifiedWindows();
  void windowAdded(EffectWindow *window);
  void windowClosed(EffectWindow *window);
  bool isApplicationWindow(Window *window) const;
  bool isWindowProtectable(Window *window) const;
  bool isTabSwitcher(Window *window) const;
  void updateCaptureWindowSelection();
  QString applicationIdentifier(Window *window) const;
  bool matchesPattern(const QString &value, const QString &pattern) const;
  bool isAllowlisted(Window *window) const;
  std::optional<bool> hasActiveTrigger() const;
  void parseTriggers();

  void emitPropertiesChanged(const QVariantMap &changed);

  bool _manualEnabled = false;
  bool _manualDisabled = false;
  bool _active = false;
  bool _protectDesktop = false;
  bool _protectPanels = false;
  bool _protectTabSwitcher = false;
  bool _backendAvailable = false;
  QStringList _allowlist;
  QStringList _triggerStrings;
  QList<Trigger> _triggers;
  QHash<Window *, bool> _preActiveCaptureState;
};
} // namespace KWin
