from PySide6.QtWidgets import QApplication, QMainWindow, QLabel, QWidget, QGridLayout, QPushButton, QGraphicsDropShadowEffect
from PySide6.QtCore import Qt
from PySide6.QtGui import QShortcut, QKeySequence


class MainWindow(QMainWindow):
    def __init__(self):
        super().__init__()
        self.setFocusPolicy(Qt.StrongFocus)
        self.setWindowTitle("hello world app")
        # self.keys = []
        self.last_checked = None
        qt_keys = [Qt.Key_1, Qt.Key_2, Qt.Key_3, Qt.Key_4, Qt.Key_5, Qt.Key_6, Qt.Key_7, Qt.Key_8, Qt.Key_9]
        container = QWidget()
        container.setStyleSheet("background: #f6f2fa;")
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
            "QPushButton:checked { background: #e6f1f0; border-color: #9aa7b3; }"
        )

        for r in range(3):
            for c in range(3):
                key = QPushButton(f"{r*3 + c + 1}")
                key.setMinimumSize(btn_size, btn_size)
                key.setMaximumSize(btn_size, btn_size)
                key.setStyleSheet(key_style)
                key.setCheckable(True)
                key.toggled.connect(self.on_toggle)
                # self.keys.append(key)
                

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

        # white outline frame around the key grid
        key_frame = QWidget()
        frame_margin = 6
        key_frame.setStyleSheet(
            "background: white;"
            "border-radius: 16px;"
        )
        frame_layout = QGridLayout(key_frame)
        frame_layout.setContentsMargins(frame_margin, frame_margin, frame_margin, frame_margin)
        frame_layout.addWidget(key_panel, 0, 0, alignment=Qt.AlignCenter)

        panel_layout = QGridLayout(key_panel)
        panel_layout.setContentsMargins(grey_cushion, grey_cushion, grey_cushion, grey_cushion)
        panel_layout.addWidget(key_surface, 0, 0, alignment=Qt.AlignCenter)

        # key map list box underneath: white fill with blue outline
        map_width = 500
        map_height = 320
        map_border = 2
        map_radius = 12

        map_box = QWidget()
        map_box.setStyleSheet(
            f"background: white;"  # keep map area bright to reduce overall grey
            f"border: {map_border}px solid rgba(126, 171, 214, 0.35);"
            f"border-radius: {map_radius}px;"
        )
        map_box.setFixedSize(map_width, map_height)

        # column layout inside map box (cells 1..9)
        map_layout = QGridLayout(map_box)
        map_layout.setContentsMargins(10, 10, 10, 10)
        map_layout.setHorizontalSpacing(0)
        map_layout.setVerticalSpacing(0)

        base_cell_style = (
            "QLabel {"
            "  background: white;"
            "  border-left: 1px solid #b7c2cc;"
            "  border-right: 1px solid #b7c2cc;"
            "  border-bottom: 1px solid #cbd5e1;"
            "  color: #1f2d3a;"
            "  font-size: 15px;"
            "  font-weight: 600;"
            "  padding: 8px 12px;"
            "}"
        )

        first_cell_style = (
            "QLabel {"
            "  background: white;"
            "  border: 1px solid #b7c2cc;"
            "  border-bottom: 1px solid #cbd5e1;"
            "  border-top-left-radius: 8px;"
            "  border-top-right-radius: 8px;"
            "  color: #1f2d3a;"
            "  font-size: 15px;"
            "  font-weight: 600;"
            "  padding: 8px 12px;"
            "}"
        )

        last_cell_style = (
            "QLabel {"
            "  background: white;"
            "  border-left: 1px solid #b7c2cc;"
            "  border-right: 1px solid #b7c2cc;"
            "  border-bottom: 1px solid #b7c2cc;"
            "  border-bottom-left-radius: 8px;"
            "  border-bottom-right-radius: 8px;"
            "  color: #1f2d3a;"
            "  font-size: 15px;"
            "  font-weight: 600;"
            "  padding: 8px 12px;"
            "}"
        )

        for i in range(9):
            cell = QLabel(str(i + 1))
            # left-align with a small inset for clarity
            cell.setAlignment(Qt.AlignVCenter | Qt.AlignLeft)
            cell.setContentsMargins(6, 0, 0, 0)
            if i == 0:
                cell.setStyleSheet(first_cell_style)
            elif i == 8:
                cell.setStyleSheet(last_cell_style)
            else:
                cell.setStyleSheet(base_cell_style)
            map_layout.addWidget(cell, i, 0)

        layout.addWidget(key_frame, 1, 1, 1, 2, alignment=Qt.AlignCenter)
        layout.addWidget(map_box, 2, 1, 1, 2, alignment=Qt.AlignCenter)
        # Example shortcut: on Enter, clear the last toggled button (if any)
        QShortcut(QKeySequence(Qt.Key_Return), self, activated=self.clear_last_checked)
        for i, key in enumerate(qt_keys):
            QShortcut(QKeySequence(key), self,
                    activated=lambda i=i: self.on_toggle)



    def on_toggle(self, checked):
        btn = self.sender()
        if checked:
            self.last_checked = btn
        elif not checked:
            btn.isChecked(True)
        # optional: do something when unchecked as well

    def clear_last_checked(self):
        if self.last_checked:
            self.last_checked.setChecked(False)
            self.last_checked = None


app = QApplication()
window = MainWindow()
window.resize(800, 600)
window.show()

app.exec()
