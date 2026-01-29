
import os

file_path = "src/MainWindow.cpp"

with open(file_path, "r", encoding="utf-8", errors="ignore") as f:
    lines = f.readlines()

# Find the first occurrence of the namespace closer
cut_index = -1
for i, line in enumerate(lines):
    if "} // namespace GOL" in line:
        cut_index = i
        break

if cut_index != -1:
    # Keep up to (but not including) the closer, so we stay inside namespace?
    # No, usually the closer is the last line.
    # We want to add methods BEFORE the closer.
    # So we take lines[:cut_index]
    
    new_methods = """
void MainWindow::mousePressEvent(QMouseEvent *event) {
  if (event->button() == Qt::LeftButton) {
    // Check if within top header height (50px)
    if (event->position().y() < 50) {
      m_dragPos = event->globalPosition().toPoint() - frameGeometry().topLeft();
      event->accept();
    }
  }
  QMainWindow::mousePressEvent(event);
}

void MainWindow::mouseMoveEvent(QMouseEvent *event) {
  if (event->buttons() & Qt::LeftButton) {
     // We assume if dragging started, we continue
     if (event->position().y() < 50) { 
        move(event->globalPosition().toPoint() - m_dragPos);
        event->accept();
     }
  }
  QMainWindow::mouseMoveEvent(event);
}

} // namespace GOL
"""
    final_content = lines[:cut_index]
    # Join and append new methods
    content_str = "".join(final_content) + new_methods
    
    with open(file_path, "w", encoding="utf-8") as f:
        f.write(content_str)
    print("Fixed MainWindow.cpp")
else:
    print("Could not find namespace closer")
