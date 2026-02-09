from PySide6.QtWidgets import QApplication, QMainWindow, QLabel, QWidget, QGridLayout, QPushButton, QGraphicsDropShadowEffect
from PySide6.QtCore import Qt

class MainWindow(QMainWindow):
    def __init__(self):
        super().__init__()
        self.setWindowTitle("hello world app")

        container = QWidget()
        self.setCentralWidget(container)

        layout = QGridLayout(container)

        # label1 = QLabel("hello world1")
        # label1.setAlignment(Qt.AlignCenter)
        # label2 = QLabel("hello world2")
        # label2.setAlignment(Qt.AlignCenter)
        # label3 = QLabel("hello world3")
        # label3.setAlignment(Qt.AlignCenter)

        # button = QPushButton("click me")
        # button.clicked.connect(do_something)

        # layout.addWidget(label1, 0, 0)
        # layout.addWidget(label2, 0, 1)
        # layout.addWidget(label3, 1, 1)
        # layout.addWidget(button, 1, 0)

        # 3x3 keyboard-style buttons with subtle shadow
        key_grid = QGridLayout()
        key_grid.setHorizontalSpacing(10)
        key_grid.setVerticalSpacing(10)

        key_style = (
            "QPushButton {"
            "  background: #f9fbfb;"
            "  border: 1.5px solid #b7c2cc;"  # soft grey outline
            "  border-radius: 10px;"
            "  color: #1f2d3a;"
            "  font-size: 16px;"
            "  font-weight: 600;"
            "  padding: 10px 12px;"
            "}"
            "QPushButton:pressed {"
            "  background: #e6f1f0;"
            "  border-color: #9aa7b3;"
            "}"
        )

        for r in range(3):
            for c in range(3):
                key = QPushButton(f"Key {r*3 + c + 1}")
                key.setMinimumSize(64, 64)
                key.setStyleSheet(key_style)

                shadow = QGraphicsDropShadowEffect()
                shadow.setBlurRadius(18)
                shadow.setOffset(0, 4)
                shadow.setColor(Qt.black)
                key.setGraphicsEffect(shadow)

                key_grid.addWidget(key, r, c)

        layout.addLayout(key_grid, 2, 0, 1, 2)

def do_something():
    print("hello")
    


app = QApplication()
window = MainWindow()
window.resize(800, 600)
window.show()

app.exec()
