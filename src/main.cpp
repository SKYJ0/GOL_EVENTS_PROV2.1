
#include "MainWindow.h"
#include "SecurityManager.h"
#include <QApplication>

int main(int argc, char *argv[]) {
  QApplication app(argc, argv);

  // Global Security Check at Startup
  GOL::SecurityManager::instance().checkAndAct();

  GOL::MainWindow window;
  window.show();

  return app.exec();
}
