from PySide6.QtWidgets import QApplication, QMainWindow, QLabel, QWidget, QGridLayout, QPushButton, QGraphicsDropShadowEffect
from PySide6.QtCore import Qt

class MainWindow(QMainWindow):
    def __init__(self):
        super().__init__()
        self.setWindowTitle("hello world app")

        container = QWidget()
        container.setStyleSheet("background: white;")
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

        # 3x3 keyboard-style buttons with layered background
        key_panel = QWidget()
        key_panel.setStyleSheet(
            "background: #e9eef2;"
            "border-radius: 14px;"
        )

        btn_size = 70
        spacing = 0
        grey_cushion = 10   # outer grey cushion
        blue_cushion = 3   # inner translucent blue cushion

        # inner translucent blue surface that sits above the grey panel
        key_surface = QWidget()
        key_surface.setStyleSheet(
            "background: rgba(126, 171, 214, 0.34);"  # soft blue tint
            "border-radius: 12px;"
        )

        key_grid = QGridLayout(key_surface)
        key_grid.setContentsMargins(blue_cushion, blue_cushion, blue_cushion, blue_cushion)
        key_grid.setHorizontalSpacing(spacing)
        key_grid.setVerticalSpacing(spacing)

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
                key = QPushButton(f"{r*3 + c + 1}")
                key.setMinimumSize(btn_size, btn_size)
                key.setMaximumSize(btn_size, btn_size)
                key.setStyleSheet(key_style)

                shadow = QGraphicsDropShadowEffect()
                shadow.setBlurRadius(18)
                shadow.setOffset(0, 4)
                shadow.setColor(Qt.black)
                key.setGraphicsEffect(shadow)

                key_grid.addWidget(key, r, c)

        blue_w = blue_cushion * 2 + btn_size * 3 + spacing * 2
        blue_h = blue_cushion * 2 + btn_size * 3 + spacing * 2
        key_surface.setFixedSize(blue_w, blue_h)

        panel_w = grey_cushion * 2 + blue_w
        panel_h = grey_cushion * 2 + blue_h
        key_panel.setFixedSize(panel_w, panel_h)

        panel_layout = QGridLayout(key_panel)
        panel_layout.setContentsMargins(grey_cushion, grey_cushion, grey_cushion, grey_cushion)
        panel_layout.addWidget(key_surface, 0, 0, alignment=Qt.AlignCenter)

        layout.addWidget(key_panel, 2, 0, 1, 2, alignment=Qt.AlignCenter)

def do_something():
    print("hello")
    


app = QApplication()
window = MainWindow()
window.resize(800, 600)
window.show()

app.exec()
