"""Live viewer for odin-data.

This demo utility displays images generated by the live view plugin of odin-data.

Tim Nicholls, STFC Application Engineering Group.
"""
import sys
import os
import random
import zmq
import signal
import argparse

from PyQt5 import QtCore, QtWidgets
import numpy as np

from mplplot import MplPlotCanvas, MplNavigationToolbar
from framestatsbar import FrameStatisticsBar

from descrambler import descramble_to_crs_fn_gn


class LiveViewerDefaults(object):
    """
    Default parameters for the live viewer.

    This class implements a simple container of default parameters used
    for the live viewer.
    """

    def __init__(self):
        """Initialise default parameters."""
        self.endpoint_url = "tcp://127.0.0.1:5020"

        self.left = 10
        self.top = 10
        self.width = 600
        self.height = 500
        self.title = 'ODIN Data Live View'


class LiveViewReceiver():
    """
    ZeroMQ-based receiver class for the live viewer.

    This class implements the ZeroMQ channel receiver for the live viewer.
    Receiving frames is socket event-driven to allow integration with the QT
    notifier mechanism.
    """

    def __init__(self, endpoint_url):
        """Initialise the receiver."""
        # Create a ZeroMQ context, socket and connect the specified endpoint
        self.context = zmq.Context()
        self.socket = self.context.socket(zmq.SUB)
        self.socket.setsockopt_string(zmq.SUBSCRIBE, "")
        self.socket.connect(endpoint_url)

        # Set up socket operation parameters
        self.zmq_flags = 0
        self.zmq_copy = True
        self.zmq_track = False

        self.debug_socket = False

        # Initialise the frames received counter
        self.frames_received = 0

    def receive_frame(self):
        """
        Receive frames from the ZeroMQ channel.

        This method receives frames from the ZeroMQ channel. This is event-driven and receives
        all pending image header/body multi-part messages before returning the latest one. This
        allows the viewer to drop frames when it is unable to display them at the rated transmitted.
        """

        # Set up default header and frame data variables
        header = {}
        frame_data = None

        # Get the socket event flags
        flags = self.socket.getsockopt(zmq.EVENTS)

        # Keep handling socket events until all flags are cleared. This is necessary as the
        # QT socket notifier is edge-triggered so the socket must be cleared of pending
        # messages before returning.
        while flags != 0:

            if flags & zmq.POLLIN:

                # Receive the live view header first
                header = self.socket.recv_json(flags=self.zmq_flags)
                if self.debug_socket:
                    print("[Socket] received header: " + repr(header))

                if header['dtype'] == 'unknown':
                    header['dtype'] = 'uint16'

                # Receive the image data payload and convert into a numpy array
                # with the appropriate shape
                msg = self.socket.recv(
                    flags=self.zmq_flags, copy=self.zmq_copy, track=self.zmq_track)
                buf = memoryview(msg)
                array = np.frombuffer(buf, dtype=header['dtype'])
                frame_data = array.reshape([int(header["shape"][0]),
                                           int(header["shape"][1])])

                frame_data = descramble_to_crs_fn_gn(frame_data,
                                                     False, # refColH1_0_Flag
                                                     True,  # cleanmem
                                                     False) # verbose


                if self.debug_socket:
                    print("[Socket] recevied frame shape: " + repr(frame_data.shape))

                # Increment frames received counter
                self.frames_received += 1

            elif flags & zmq.POLLOUT:
                if self.debug_socket:
                    print("[Socket] zmq.POLLOUT")
            elif flags & zmq.POLLERR:
                if self.debug_socket:
                    print("[Socket] zmq.POLLERR")
            else:
                if self.debug_socket:
                    print("[Socket] FAILURE")

            # Update socket event flags to drive loop
            flags = self.socket.getsockopt(zmq.EVENTS)
            if self.debug_socket:
                print("Flags at end", flags)

        # Return the most receently received header and frame data
        return (header, frame_data)

    def get_socket_fd(self):
        """Return the ZeroMQ channel socket file descriptor."""
        return self.socket.getsockopt(zmq.FD)

    def reset_statistics(self):
        """Reset frames received statistics."""
        self.frames_received = 0


class LiveViewer(QtWidgets.QMainWindow):
    """
    Live viewer class.

    The class implements the main logic of the live viewer application and the associated
    user interface.
    """

    def __init__(self, args):
        """
        Initialise the live view.

        This method initialises the liew viewer application and user interface
        """
        super().__init__()

        # Create a default parameter object and parse command-line arguments
        self.defaults = LiveViewerDefaults()
        self.args = self._parse_arguments(args)

        # Initialise frames shown counter
        self.frames_shown = 0

        # Initialise the main window user interface
        self.init_ui()

        # Create the receiver object
        self.receiver = LiveViewReceiver(self.args.endpoint_url)

        # Create a socket event notifier attached to the receiver socket and connect
        # to the receive handler
        self.notifier = QtCore.QSocketNotifier(
            self.receiver.get_socket_fd(), QtCore.QSocketNotifier.Read, self
        )
        self.notifier.activated.connect(self.handle_receive)

    def _parse_arguments(self, args=None):
        """
        Parse command line arguments.

        Parse command line arugments, using the default parameter object to set default
        values.
        """

        # First argument passed after QApplication processes sys.argv is the
        # progam name
        prog_name=args.pop(0)

        # Set the terminal width for argument help formatting
        try:
            term_columns = int(os.environ['COLUMNS']) - 2
        except (KeyError, ValueError):
            term_columns = 100

        # Build options for the argument parser
        parser = argparse.ArgumentParser(
            prog=prog_name, description='ODIN data live viewer',
            formatter_class=lambda prog: argparse.ArgumentDefaultsHelpFormatter(
                prog, max_help_position=40, width=term_columns)
        )

        parser.add_argument('--endpoint', type=str, dest="endpoint_url",
                            default=self.defaults.endpoint_url,
                            help='ZeroMQ endpoint URL to connect to')

        # Parse and return arguments
        parsed_args = parser.parse_args(args)
        return parsed_args

    def init_ui(self):
        """
        Initialise the user interface.

        This method initialises the QT main window interface, setting up the various widgets
        and shwoing the window.
        """
        # Set window title and geometry
        self.setWindowTitle(self.defaults.title)
        self.setGeometry(
            self.defaults.left, self.defaults.top, self.defaults.width, self.defaults.height)

        # Create a main widget
        self.main_widget = QtWidgets.QWidget(self)

        # Create a vertical box layout
        vbl = QtWidgets.QVBoxLayout(self.main_widget)

        # Instantiate a Matplotlib canvas
        self.plot = MplPlotCanvas(self.main_widget)

        # Instantiate the navigation toolbar for the plot
        self.nav_tool_bar = MplNavigationToolbar(self.plot, self.main_widget)

        # Instantiate and configure the frame statistics bar
        self.stats_bar = FrameStatisticsBar(self.main_widget)
        self.stats_bar.connect_reset(self.on_reset_stats)

        # Pack widgets into the vertical box layout
        vbl.addWidget(self.nav_tool_bar)
        vbl.addWidget(self.plot)
        vbl.addWidget(self.stats_bar)
        vbl.setContentsMargins(0, 0, 0, 0)
        vbl.setSpacing(0)

        # Set focus on the main widget
        self.main_widget.setFocus()

        # Set the central widget of the main window to main_widget
        self.setCentralWidget(self.main_widget)

        # Show the window
        self.show()

    def handle_receive(self):
        """
        Handle receive events.

        This method handles receive events generated by the socket notifier,
        calling the receiver and rendering any frames returned.
        """

        # Disable notifier while handling
        self.notifier.setEnabled(False)

        # Get frame data from the receiver
        (header, frame_data) = self.receiver.receive_frame()

        # Plot frame data if returned
        if frame_data is not None:
            self.plot.multi_render_frame(frame_data)

            self.frames_shown += 1

        # Update statistics bar
        self.stats_bar.update(self.receiver.frames_received, self.frames_shown)

        # Re-enable the notifier
        self.notifier.setEnabled(True)

    @QtCore.pyqtSlot()
    def on_reset_stats(self):
        """
        Reset the frame statistics displayed in the view viewer.

        This method implements a slot called to reset the viewer frame statistics
        when the reset button is pressed.
        """

        # Reset frames stats in this object and in receiver
        self.receiver.reset_statistics()
        self.frames_shown = 0

        # Update the stastics bar widget
        self.stats_bar.update(self.receiver.frames_received, self.frames_shown)


def main():
    """Main application entry point."""

    # Bind Ctrl-C INT signal to default to make work in the app GUI
    signal.signal(signal.SIGINT, signal.SIG_DFL)

    # Construct QT app and viewer objects
    app = QtWidgets.QApplication(sys.argv)
    live_viewer = LiveViewer(app.arguments())

    # Run the app to completion
    sys.exit(app.exec_())


if __name__ == '__main__':
    main()