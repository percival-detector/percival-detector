"""Frame statistics bar widget for live viewer.

This class implememnts a PyQT widget for displaying frame statistics
in the live viewer application.

Tim Nicholls, STFC Application Engineering Group.
"""
from PyQt5 import QtWidgets

class FrameStatisticsBar(QtWidgets.QWidget):

    def __init__(self, parent=None):
        """
        Frame statistics bar widget for live viewer.

        This class implememnts a PyQT widget for displaying frame statistics
        in the live viewer application.
        """

        # Initialise super class
        super(FrameStatisticsBar, self).__init__(parent)

        # Create box layout and widgets
        hbl = QtWidgets.QHBoxLayout(self)
        self.label_recvd = QtWidgets.QLabel("Frames recvd:")
        self.frames_recvd = QtWidgets.QLabel("0")
        self.frames_recvd.setFixedWidth(30)
        self.label_shown = QtWidgets.QLabel("Frames shown:")
        self.frames_shown = QtWidgets.QLabel("0")

        self.reset_button = QtWidgets.QPushButton("Reset")
    
        # Pack widgets into layout
        hbl.addWidget(self.label_recvd)
        hbl.addWidget(self.frames_recvd)
        hbl.addWidget(self.label_shown)
        hbl.addWidget(self.frames_shown)
        hbl.addStretch()
        hbl.addWidget(self.reset_button)

    def update(self, frames_recvd, frames_shown):
        """Update the counters and repaint."""
        self.frames_recvd.setText(str(frames_recvd))
        self.frames_recvd.repaint()
        self.frames_shown.setText(str(frames_shown))
        self.frames_shown.repaint()
        
    def connect_reset(self, slot):
        """Connect the reset button to the specified slot."""
        self.reset_button.clicked.connect(slot)