kcoreaddons_add_plugin(securecapture SOURCES effect.cpp
    INSTALL_NAMESPACE "kwin/effects/plugins")
set_property(TARGET securecapture APPEND PROPERTY AUTOMOC_MACRO_NAMES "KWIN_EFFECT_FACTORY")
target_include_directories(securecapture PRIVATE ${KWIN_INCLUDE_DIR})
target_link_libraries(securecapture PRIVATE
    ${KWIN_LIBRARY}
    Qt6::Core
    Qt6::DBus
    Qt6::Gui
    Qt6::Qml
    Qt6::Widgets
    KF6::ConfigCore
    KF6::CoreAddons
)
