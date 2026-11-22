#include "effect.h"

#include <kwin/effect/effecthandler.h>
#include <kwin/effect/effectwindow.h>
#include <kwin/window.h>

#include <KConfigGroup>
#include <KSharedConfig>

#include <QDBusConnection>
#include <QDBusMessage>
#include <QDebug>
#include <QMetaProperty>
#include <QRegularExpression>
#include <qlogging.h>

namespace {
const auto serviceName = QStringLiteral("org.kde.SecureCapture");
const auto objectPath = QStringLiteral("/SecureCapture");
const auto configGroup = QStringLiteral("SecureCapture");

bool validPattern(const QString &pattern) {
  return !pattern.isEmpty() && !pattern.contains(QLatin1Char('\n')) &&
         !pattern.contains(QLatin1Char('\r'));
}

#ifndef QT_NO_DEBUG
inline void dumpProperties(const QObject *obj, const char *reason) {
  qDebug() << reason;

  const QMetaObject *mo = obj->metaObject();
  qDebug().nospace() << "=== " << mo->className() << " @ " << obj << " ===";

  for (int i = 0; i < mo->propertyCount(); ++i) {
    const QMetaProperty p = mo->property(i);
    qDebug().nospace() << "  " << p.name() << " = " << p.read(obj);
  }
  for (const QByteArray &name : obj->dynamicPropertyNames()) {
    qDebug().nospace() << "  [dyn] " << name << " = "
                       << obj->property(name.constData());
  }
}
#define DUMP_PROPS(obj, reason) dumpProperties(obj, reason)
#else
#define DUMP_PROPS(obj, reason)                                                \
  do {                                                                         \
  } while (0)
#endif
} // namespace

namespace KWin {
SecureCaptureEffect::SecureCaptureEffect() {
  connect(effects, &EffectsHandler::windowAdded, this,
          &SecureCaptureEffect::windowAdded);
  connect(effects, &EffectsHandler::windowDeleted, this,
          &SecureCaptureEffect::windowClosed);

  loadConfiguration();
  parseTriggers();

  QDBusConnection bus = QDBusConnection::sessionBus();
  _backendAvailable = bus.registerService(serviceName);
  if (_backendAvailable) {
    const bool objectRegistered =
        bus.registerObject(objectPath, this,
                           QDBusConnection::ExportScriptableSlots |
                               QDBusConnection::ExportAllProperties);
    if (!objectRegistered) {
      bus.unregisterService(serviceName);
      _backendAvailable = false;
    }
  }

  updateState();

  emitPropertiesChanged(
      {{QStringLiteral("backendAvailable"), _backendAvailable}});
}

SecureCaptureEffect::~SecureCaptureEffect() {
  restoreModifiedWindows();
  QDBusConnection bus = QDBusConnection::sessionBus();
  if (_backendAvailable) {
    bus.unregisterObject(objectPath);
    bus.unregisterService(serviceName);
    emitPropertiesChanged({{QStringLiteral("backendAvailable"), false}});
  }
}

bool SecureCaptureEffect::active() const { return _active; }

bool SecureCaptureEffect::manualEnabled() const { return _manualEnabled; }

bool SecureCaptureEffect::manualDisabled() const { return _manualDisabled; }

bool SecureCaptureEffect::protectDesktop() const { return _protectDesktop; }

bool SecureCaptureEffect::protectPanels() const { return _protectPanels; }

bool SecureCaptureEffect::protectTabSwitcher() const {
  return _protectTabSwitcher;
}

bool SecureCaptureEffect::backendAvailable() const { return _backendAvailable; }

QStringList SecureCaptureEffect::allowlist() const { return _allowlist; }

QStringList SecureCaptureEffect::triggers() const { return _triggerStrings; }

void SecureCaptureEffect::setEnabled(bool enabled) {
  if (_manualEnabled == enabled) {
    return;
  }
  _manualEnabled = enabled;
  saveConfiguration();
  updateState();

  emitPropertiesChanged({{QStringLiteral("manualEnabled"), _manualEnabled}});
}

void SecureCaptureEffect::setDisabled(bool disabled) {
  if (_manualDisabled == disabled) {
    return;
  }
  _manualDisabled = disabled;
  updateState();

  emitPropertiesChanged({{QStringLiteral("manualDisabled"), _manualDisabled}});
}

void SecureCaptureEffect::setProtectDesktop(bool protect) {
  if (_protectDesktop == protect) {
    return;
  }
  _protectDesktop = protect;
  saveConfiguration();
  updateCaptureWindowSelection();

  emitPropertiesChanged({{QStringLiteral("protectDesktop"), _protectDesktop}});
}

void SecureCaptureEffect::setProtectPanels(bool protect) {
  if (_protectPanels == protect) {
    return;
  }
  _protectPanels = protect;
  saveConfiguration();
  updateCaptureWindowSelection();

  emitPropertiesChanged({{QStringLiteral("protectPanels"), _protectPanels}});
}

void SecureCaptureEffect::setProtectTabSwitcher(bool protect) {
  if (_protectTabSwitcher == protect) {
    return;
  }
  _protectTabSwitcher = protect;
  saveConfiguration();
  updateCaptureWindowSelection();

  emitPropertiesChanged(
      {{QStringLiteral("protectTabSwitcher"), _protectTabSwitcher}});
}

void SecureCaptureEffect::setAllowlist(const QStringList &patterns) {
  QStringList filtered;
  for (const QString &pattern : patterns) {
    const QString trimmed = pattern.trimmed();
    if (validPattern(trimmed) && !filtered.contains(trimmed)) {
      filtered.append(trimmed);
    }
  }
  if (_allowlist == filtered) {
    return;
  }
  _allowlist = filtered;
  saveConfiguration();
  if (_active) {
    updateCaptureWindowSelection();
  }

  emitPropertiesChanged({{QStringLiteral("allowlist"), _allowlist}});
}

void SecureCaptureEffect::setTriggers(const QStringList &rules) {
  QStringList filtered;
  for (const QString &rule : rules) {
    const QStringList fields = rule.split(QLatin1Char('\t'));
    if (fields.size() == 2 && validPattern(fields.at(0)) &&
        (fields.at(1) == QLatin1String("enable") ||
         fields.at(1) == QLatin1String("disable"))) {
      filtered.append(fields.at(0).trimmed() + QLatin1Char('\t') +
                      fields.at(1));
    }
  }
  if (_triggerStrings == filtered) {
    return;
  }
  _triggerStrings = filtered;
  parseTriggers();
  saveConfiguration();
  updateState();

  emitPropertiesChanged({{QStringLiteral("triggers"), _triggerStrings}});
}

void SecureCaptureEffect::loadConfiguration() {
  const KConfigGroup group(KSharedConfig::openConfig(), configGroup);
  _manualEnabled = group.readEntry("ManualEnabled", false);
  const bool protectEssentialWindows =
      group.readEntry("ProtectEssentialWindows", false);
  _protectDesktop = group.hasKey("ProtectDesktop")
                        ? group.readEntry("ProtectDesktop", false)
                        : protectEssentialWindows;
  _protectPanels = group.hasKey("ProtectPanels")
                       ? group.readEntry("ProtectPanels", false)
                       : protectEssentialWindows;
  _protectTabSwitcher = group.readEntry("ProtectTabSwitcher", false);
  _allowlist = group.readEntry("Allowlist", QStringList{});
  _triggerStrings = group.readEntry("Triggers", QStringList{});
}

void SecureCaptureEffect::saveConfiguration() {
  KConfigGroup group(KSharedConfig::openConfig(), configGroup);
  group.writeEntry("ManualEnabled", _manualEnabled);
  group.writeEntry("ProtectDesktop", _protectDesktop);
  group.writeEntry("ProtectPanels", _protectPanels);
  group.writeEntry("ProtectTabSwitcher", _protectTabSwitcher);
  group.writeEntry("Allowlist", _allowlist);
  group.writeEntry("Triggers", _triggerStrings);
  group.sync();
}

void SecureCaptureEffect::parseTriggers() {
  _triggers.clear();
  for (const QString &rule : _triggerStrings) {
    const QStringList fields = rule.split(QLatin1Char('\t'));
    if (fields.size() == 2) {
      _triggers.append({fields.at(0), fields.at(1) == QLatin1String("enable")});
    }
  }
}

std::optional<bool> SecureCaptureEffect::hasActiveTrigger() const {
  std::optional<bool> triggerState;

  for (EffectWindow *effectWindow : effects->stackingOrder()) {
    Window *window = effectWindow->window();
    if (!isApplicationWindow(window)) {
      continue;
    }
    const QString identifier = applicationIdentifier(window);
    for (const Trigger &trigger : _triggers) {
      if (matchesPattern(identifier, trigger.pattern)) {
        if (!triggerState.has_value()) {
          triggerState = trigger.enable;
        }
        if (trigger.enable == false) {
          return false;
        }
      }
    }
  }

  return triggerState;
}

void SecureCaptureEffect::updateState() {
  const bool oldActive = _active;
  if (_manualDisabled) {
    _active = false;
  } else {
    auto triggerState = hasActiveTrigger();
    if (triggerState.has_value()) {
      _active = triggerState.value();
    } else {
      _active = _manualEnabled;
    }
  }

  if (oldActive != _active) {
    if (_active) {
      applyToAllWindows();
    } else {
      restoreModifiedWindows();
    }

    emitPropertiesChanged({{QStringLiteral("active"), _active}});
  }
}

bool SecureCaptureEffect::isApplicationWindow(Window *window) const {
  if (window == nullptr || window->isDesktop() || window->isDock() ||
      window->isLockScreen() || window->isInputMethod() ||
      window->isOutline() || isTabSwitcher(window)) {
    return false;
  }
  return window->isNormalWindow() || window->isDialog() ||
         window->isUtility() || window->isSplash();
}

bool SecureCaptureEffect::isWindowProtectable(Window *window) const {
  if (isApplicationWindow(window)) {
    return true;
  }
  if (window == nullptr) {
    return false;
  }
  return (_protectDesktop && window->isDesktop()) ||
         (_protectPanels && (window->isDock() || window->isAppletPopup())) ||
         (_protectTabSwitcher && isTabSwitcher(window));
}

bool SecureCaptureEffect::isTabSwitcher(Window *window) const {
  return window && window->isNormalWindow() && window->skipSwitcher() &&
         window->layer() == OverlayLayer &&
         window->desktopFileName().isEmpty() &&
         window->resourceName().isEmpty() && window->pid() <= 0;
}

void SecureCaptureEffect::updateCaptureWindowSelection() {
  for (auto it = _preActiveCaptureState.begin();
       it != _preActiveCaptureState.end();) {
    if (!isWindowProtectable(it.key())) {
      if (it.key() != nullptr) {
        it.key()->setExcludeFromCapture(it.value());
      }
      it = _preActiveCaptureState.erase(it);
    } else {
      ++it;
    }
  }
  if (_active) {
    applyToAllWindows();
  }
}

QString SecureCaptureEffect::applicationIdentifier(Window *window) const {
  const QString desktopId = window->desktopFileName();
  return desktopId.isEmpty() ? window->resourceClass() : desktopId;
}

bool SecureCaptureEffect::matchesPattern(const QString &value,
                                         const QString &pattern) const {
  QString expression;
  const QStringList parts = pattern.split(QLatin1Char('*'));
  for (qsizetype i = 0; i < parts.size(); ++i) {
    if (i > 0) {
      expression += QStringLiteral(".*");
    }
    expression += QRegularExpression::escape(parts.at(i));
  }
  return QRegularExpression(QStringLiteral("\\A") + expression +
                            QStringLiteral("\\z"))
      .match(value)
      .hasMatch();
}

bool SecureCaptureEffect::isAllowlisted(Window *window) const {
  const QString identifier = applicationIdentifier(window);
  for (const QString &pattern : _allowlist) {
    if (matchesPattern(identifier, pattern)) {
      return true;
    }
  }
  return false;
}

void SecureCaptureEffect::applyToWindow(EffectWindow *effectWindow) {
  if (!_active) {
    return;
  }
  Window *window = effectWindow != nullptr ? effectWindow->window() : nullptr;
  if (!isWindowProtectable(window)) {
    return;
  }

  if (!_preActiveCaptureState.contains(window)) {
    _preActiveCaptureState.insert(window, window->excludeFromCapture());
  }
  window->setExcludeFromCapture(_active && !isAllowlisted(window));
}

void SecureCaptureEffect::applyToAllWindows() {
  for (EffectWindow *window : effects->stackingOrder()) {
    applyToWindow(window);
  }
}

void SecureCaptureEffect::restoreModifiedWindows() {
  for (auto it = _preActiveCaptureState.cbegin();
       it != _preActiveCaptureState.cend(); ++it) {
    if (it.key()) {
      it.key()->setExcludeFromCapture(it.value());
    }
  }
  _preActiveCaptureState.clear();
}

void SecureCaptureEffect::windowAdded(EffectWindow *window) {
  DUMP_PROPS(window->window(), "Window added: ");

  applyToWindow(window);
  updateState();
}

void SecureCaptureEffect::windowClosed(EffectWindow *effectWindow) {
  if (effectWindow) {
    _preActiveCaptureState.remove(effectWindow->window());
  }
  updateState();
}

void SecureCaptureEffect::emitPropertiesChanged(const QVariantMap &changed) {
  if (changed.isEmpty())
    return;

  QDBusMessage message = QDBusMessage::createSignal(
      objectPath, QStringLiteral("org.freedesktop.DBus.Properties"),
      QStringLiteral("PropertiesChanged"));

  message << QStringLiteral("org.kde.SecureCapture") << changed
          << QStringList{};

  if (!QDBusConnection::sessionBus().send(message)) {
    qWarning() << "Failed to emit PropertiesChanged:"
               << QDBusConnection::sessionBus().lastError().message();
  }
}

KWIN_EFFECT_FACTORY(SecureCaptureEffect, "metadata.json")
} // namespace KWin

#include "effect.moc"
