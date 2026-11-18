#pragma once

#include <functional>
#include <memory>

#include <QString>

class QGuiApplication;
class QQmlApplicationEngine;
class QObject;

namespace TagOS::Runtime {

/// Everything an application entry point has to declare about itself.
struct Options
{
    QString uri;          ///< QML module URI, e.g. "TagOS.Files"
    QString appName;      ///< process name / ready-file component, e.g. "tag-files"
    QString displayName;  ///< human name, e.g. "TagFiles"
    QString version;      ///< semver reported in --version and the ready file
    QString logPrefix;    ///< stderr prefix, defaults to appName
    /// Write the session-ready marker even without --ready-file (the shell does
    /// this: the boot marker waits for it).
    bool alwaysWriteReadyFile = false;
    /// Optional hook after the QML root object exists (shell uses it to pass
    /// the startup surface requested by TAGOS_SHELL_STATE).
    std::function<void(QQmlApplicationEngine &engine, QObject *root)> afterLoad;
};

/// Runs a TagOS Qt Quick application.
///
/// Shared behaviour, identical in every TagOS binary:
///   * QML import paths (build tree, installed prefix, TAGOS_QML_PATH),
///   * `--smoke-test` (load, render, exit 0; fails when QML logged runtime errors),
///   * `--screenshot <path>` (render one frame and exit),
///   * `--ready-file <path>` (session marker used by the boot test),
///   * `--log-file <path>` plus TAGOS_LOG_FILE for collected diagnostics.
///
/// Returns the process exit code.
int run(int argc, char *argv[], const Options &options);

} // namespace TagOS::Runtime
