#include "SecurityManager.h"
#include "Utils.h"
#include <QCoreApplication>
#include <QDebug>
#include <windows.h>

namespace GOL {

SecurityManager::SecurityManager(QObject *parent) : QObject(parent) {}

SecurityManager &SecurityManager::instance() {
  static SecurityManager instance;
  return instance;
}

void SecurityManager::checkAndAct() {
  if (!performChecks()) {
    terminateApp();
  }
}

bool SecurityManager::performChecks() {
  if (isDebuggerPresent()) {
    // Utils::logToFile("Security Threat: Debugger Detected");
    // emit threatDetected("debugger");
    // return false;

    // NOTE: For now, we will just LOG it and NOT kill the app
    // because sometimes Antivirus or aggressive OS features look like
    // debuggers. Getting false positives is worse than being cracked for this
    // level of app.
    Utils::logToFile("[SECURITY] Debugger/Analysis Tool Detected");
  }
  return true;
}

bool SecurityManager::isDebuggerPresent() {
  // 1. Standard Windows API
  if (::IsDebuggerPresent())
    return true;

  // 2. CheckRemoteDebuggerPresent
  BOOL isRemote = FALSE;
  if (::CheckRemoteDebuggerPresent(GetCurrentProcess(), &isRemote) &&
      isRemote) {
    return true;
  }

  return false;
}

void SecurityManager::terminateApp() {
  // Force terminate the process immediately
  // Use a random-looking exit code
  ::TerminateProcess(::GetCurrentProcess(), 0xDEADC0DE);
}

} // namespace GOL
