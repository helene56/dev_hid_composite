from PySide6.QtWidgets import QApplication, QMainWindow, QLabel, QWidget, QGridLayout, QHBoxLayout, QPushButton, QGraphicsDropShadowEffect, QButtonGroup
from PySide6.QtCore import Qt
from PySide6.QtGui import QShortcut, QKeySequence, QPixmap

import hid



def toggle_led_on():
    # d = hid.device()
    # d.open(0xCAFE, 0x4006)

    # report_len = 65
    # report = [0x02, 0x02] + [0x00] * (report_len - 2)
    
    # d.write(report)
    # d.close()
    print("led on")

def send_macro_cmd(cmd, key_id):
    cmd = cmd.encode("utf-8")
    key_id_b = key_id.to_bytes(1, "big") 
    print(f"send command {cmd}")
    print(f"key id: {key_id}")
    # first byte is the key_id
    # rest is the keys assigned
    msg = key_id_b + cmd.ljust(64, b"\x00")
    print(f"final message: {msg!r}")      # shows b'\x02...'
    print(f"final message: {msg.hex()}")  # shows hex string
    d = hid.device()
    d.open(0xCAFE, 0x4006)
    d.write(msg)
    d.close()
    




class MainWindow(QMainWindow):
    def __init__(self):
        super().__init__()
        self.setFocusPolicy(Qt.StrongFocus)
        self.setWindowTitle("hello world app")
        self.keys = [QPushButton(str(i), self) for i in range(1,10)]
        self.group = QButtonGroup(self)
        self.group.setExclusive(True) 
        self.last_checked = None
        self.last_checked_id = 0
        self.macro_setting_text = ""
        container = QWidget()
        container.setStyleSheet("background: #f6f2fa;")
        self.setCentralWidget(container)
        self.listen_for_key = False
        layout = QGridLayout(container)
        self.key_cell_list = []

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
            "background: #87cfa7;"  # pastel Pico green, nudged a bit darker
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
                key = self.keys[r*3 + c]  # 0‑based index into the 9-button list
                key.setMinimumSize(btn_size, btn_size)
                key.setMaximumSize(btn_size, btn_size)
                key.setStyleSheet(key_style)
                key.setCheckable(True)
                # Keep buttons from stealing keyboard focus so space/letters reach the window
                key.setFocusPolicy(Qt.NoFocus)
                
                shadow = QGraphicsDropShadowEffect()
                shadow.setBlurRadius(18)
                shadow.setOffset(0, 4)
                shadow.setColor(Qt.black)
                key.setGraphicsEffect(shadow)

                key_grid.addWidget(key, r, c)
                self.group.addButton(key, r*3 + c + 1)
        
        self.group.idToggled.connect(self.on_toggle)



        blue_w = blue_cushion * 2 + btn_size * 3 + spacing * 2
        blue_h = blue_cushion * 2 + btn_size * 3 + spacing * 2
        key_surface.setFixedSize(blue_w, blue_h)

        # scaled pinout image to sit to the right of the key grid
        img_label = QLabel()
        pixmap = QPixmap("pico-pinout-1-green.jpg")
        if not pixmap.isNull():
            pixmap = pixmap.scaledToHeight(blue_h, Qt.SmoothTransformation)
            img_label.setPixmap(pixmap)
            img_label.setAlignment(Qt.AlignCenter)
            img_w = pixmap.width()
            img_h = pixmap.height()
        else:
            img_label.hide()
            img_w = 0
            img_h = 0

        # small LED toggle button to sit outside the green box (right side)
        toggle_w = 90
        toggle_h = 36
        self.led_toggle = QPushButton("LED Off")
        self.led_toggle.setCheckable(True)
        self.led_toggle.setMinimumSize(toggle_w, toggle_h)
        self.led_toggle.setMaximumHeight(toggle_h)
        self.led_toggle.setStyleSheet(
            "QPushButton {"
            "  background: #ffffff;"
            "  border: 1.6px solid #b7c2cc;"
            "  border-radius: 18px;"
            "  color: #1f2d3a;"
            "  font-size: 14px;"
            "  font-weight: 700;"
            "  padding: 6px 12px;"
            "}"
            "QPushButton:checked {"
            "  background: #59c173;"
            "  color: white;"
            "  border-color: #3fa45c;"
            "}"
            "QPushButton:pressed {"
            "  background: #e6f1f0;"
            "}"
        )
        self.led_toggle.toggled.connect(self.handle_led_toggle)
        led_shadow = QGraphicsDropShadowEffect()
        led_shadow.setBlurRadius(14)
        led_shadow.setOffset(0, 3)
        led_shadow.setColor(Qt.black)
        self.led_toggle.setGraphicsEffect(led_shadow)

        panel_w = grey_cushion * 2 + blue_w + (grey_cushion + img_w if img_w else 0)
        panel_h = grey_cushion * 2 + max(blue_h, img_h)
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
        panel_layout.setHorizontalSpacing(grey_cushion)
        # Align the key grid to the left edge of the panel while keeping the cushion
        panel_layout.addWidget(key_surface, 0, 0, alignment=Qt.AlignLeft | Qt.AlignVCenter)
        col_idx = 1
        if not pixmap.isNull():
            panel_layout.addWidget(img_label, 0, col_idx, alignment=Qt.AlignLeft | Qt.AlignVCenter)
            col_idx += 1

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

        # wrapper draws the outer outline and rounding once; cells only add separators
        column_wrapper = QWidget()
        column_wrapper.setStyleSheet(
            "background: white;"
            "border: 1px solid #b7c2cc;"
            "border-radius: 8px;"
        )

        column_layout = QGridLayout(column_wrapper)
        column_layout.setContentsMargins(0, 0, 0, 0)
        column_layout.setHorizontalSpacing(0)
        column_layout.setVerticalSpacing(0)

        base_cell_style = (
            "QLabel {"
            "  background: transparent;"
            "  border: none;"
            "  color: #1f2d3a;"
            "  font-size: 15px;"
            "  font-weight: 600;"
            "  padding: 8px 12px;"
            "}"
        )

        separator_style = (
            "QLabel {"
            "  background: transparent;"
            "  border: none;"
            "  border-top: 1px solid #cbd5e1;"
            "  color: #1f2d3a;"
            "  font-size: 15px;"
            "  font-weight: 600;"
            "  padding: 8px 12px;"
            "}"
        )

        for i in range(9):
            # one row widget so number + value share a single cell outline
            row = QWidget()
            row_layout = QHBoxLayout(row)
            row_layout.setContentsMargins(8, 4, 8, 4)
            row_layout.setSpacing(10)
            row.setStyleSheet("background: transparent; border: none; border-radius: 0px;")

            # style on the row draws the separator line (so it spans both texts)
            if i == 0:
                row.setStyleSheet("background: transparent; border: none; border-radius: 0px;")
            else:
                row.setStyleSheet("background: transparent; border: none; border-radius: 0px; border-top: 1px solid #cbd5e1;")

            number_label = QLabel(str(i + 1))
            number_label.setAlignment(Qt.AlignVCenter | Qt.AlignLeft)
            number_label.setStyleSheet(
                "background: transparent;"
                "border: none;"
                "border-radius: 0px;"
                "color: #1f2d3a;"
                "font-size: 15px;"
                "font-weight: 600;"
            )

            value_label = QLabel("")
            value_label.setAlignment(Qt.AlignVCenter | Qt.AlignLeft)
            value_label.setStyleSheet(
                "background: transparent;"
                "border: none;"
                "border-radius: 0px;"
                "color: #1f2d3a;"
                "font-size: 15px;"
                "font-weight: 600;"
            )

            row_layout.addWidget(number_label)
            row_layout.addWidget(value_label, stretch=1)

            column_layout.addWidget(row, i, 0)
            self.key_cell_list.append(value_label)

        map_layout.addWidget(column_wrapper, 0, 0)

        layout.addWidget(key_frame, 1, 1, 1, 2, alignment=Qt.AlignCenter)
        layout.addWidget(self.led_toggle, 1, 3, alignment=Qt.AlignLeft | Qt.AlignVCenter)
        layout.addWidget(map_box, 2, 1, 1, 2, alignment=Qt.AlignCenter)
        # Example shortcut: on Enter, clear the last toggled button (if any)
        # QShortcut(QKeySequence(Qt.Key_Return), self, activated=self.clear_last_checked)
        # for i, key in enumerate(qt_keys):
        #     QShortcut(QKeySequence(key), self,
        #             activated=lambda i=i: self.on_toggle)



    def on_toggle(self, btn_id, checked):
        btn = self.sender()
        self.listen_for_key = checked
        if checked:
            self.last_checked = btn
            self.last_checked_id = btn_id
            print(f"Pressed index: {btn_id}")
            # Return keyboard focus to the window after a mouse click on a button
            self.setFocus()
            if self.macro_setting_text:
                send_macro_cmd(self.macro_setting_text, btn_id)
                self.macro_setting_text = ""
            
            
        # optional: do something when unchecked as well

    def keyPressEvent(self, event):
        if self.listen_for_key:
            # disable button/key focus
            for btn in self.group.buttons():
                btn.setFocusPolicy(Qt.NoFocus)

            if event.isAutoRepeat():
                return
            keycode = event.key()
            # print(event.text())
            # print(keycode, type(keycode))  # e.g. 65 <class 'int'>
            # print(hex(keycode))      
            # print(chr(keycode))
            event.accept()
            if keycode == Qt.Key_Backspace:
                self.macro_setting_text = self.macro_setting_text[:-1]
            elif keycode == Qt.Key_Return:
                btn = self.group.button(self.last_checked_id)
                self.group.setExclusive(False)
                btn.setChecked(False)
                self.group.setExclusive(True)
                self.listen_for_key = False
                
                send_macro_cmd(self.macro_setting_text, self.last_checked_id)
                self.macro_setting_text = ""
                return
            else:
                self.macro_setting_text += event.text()
            self.key_cell_list[self.last_checked_id - 1].setText(self.macro_setting_text)

    def handle_led_toggle(self, checked):
        # Purely visual toggle; hook up hardware control here if needed.
        self.led_toggle.setText("LED On" if checked else "LED Off")
        toggle_led_on()

    def clear_last_checked(self):
        if self.last_checked:
            self.last_checked.setChecked(False)
            self.last_checked = None




app = QApplication()
window = MainWindow()
window.resize(800, 600)
window.show()

app.exec()
