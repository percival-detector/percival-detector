"""Matplotlib-based 2D image canvas widget for ODIN data viewer.

This class implements a Matplotlib 2D image plotting canvas for use in the odin-data
live viewer. It supports efficient re-rendering of incoming images to maximise
performance.

Tim Nicholls, STFC Application Engineering Group.
"""
import numpy as np

import matplotlib
from matplotlib.backends.qt_compat import QtCore, QtWidgets, is_pyqt5

if is_pyqt5():
    from matplotlib.backends.backend_qt5agg import (
        FigureCanvas, NavigationToolbar2QT as NavigationToolbar)
else:
    from matplotlib.backends.backend_qt4agg import (
        FigureCanvas, NavigationToolbar2QT as NavigationToolbar)
from matplotlib.figure import Figure
from matplotlib.colorbar import make_axes, Colorbar

class MplPlotCanvas(FigureCanvas):
    """
    Matplot plot canvas.

    This class implements the updating 2D image canvas.
    """
    matplotlib.rcParams.update({'font.size': 8})
    cbar_numticks = 9

    def __init__(self, parent=None, dpi=100):
        """
        Initialise the plot canvas object.

        This method initialises the plot canvas object, creating an empty subplot
        within it.
        """
<<<<<<< HEAD
=======
        # Create a list of ADC part for plotting
        self.adc = ('Coarse', 'Fine', 'Gain')

>>>>>>> live_viewer
        # Create the figure canvas
        self.figure = Figure(dpi=dpi)
        FigureCanvas.__init__(self, self.figure)
        self.setParent(parent)
        FigureCanvas.setSizePolicy(self,
<<<<<<< HEAD
                                   QtWidgets.QSizePolicy.Expanding,
                                   QtWidgets.QSizePolicy.Expanding)
        FigureCanvas.updateGeometry(self)

        # Add the image axes to the figure
        self.axes_gain = self.figure.add_subplot(131)
        self.axes_coarse = self.figure.add_subplot(132)
        self.axes_fine = self.figure.add_subplot(133)
        self.axes_gain.set_xticks([])
        self.axes_gain.set_yticks([])
        self.axes_coarse.set_xticks([])
        self.axes_coarse.set_yticks([])
        self.axes_fine.set_xticks([])
        self.axes_fine.set_yticks([])
=======
                QtWidgets.QSizePolicy.Expanding,
                QtWidgets.QSizePolicy.Expanding)
        FigureCanvas.updateGeometry(self)
        
        # Add the image axes to the figure
        self.axes = (self.figure.add_subplot(1, 3, 1),
                   self.figure.add_subplot(1, 3, 2),
                   self.figure.add_subplot(1, 3, 3))

        self.axes[0].set_xticks([])
        self.axes[0].set_yticks([])
        self.axes[0].set_title(self.adc[0])
        self.axes[1].set_xticks([])
        self.axes[1].set_yticks([])
        self.axes[1].set_title(self.adc[1])
        self.axes[2].set_xticks([])
        self.axes[2].set_yticks([])
        self.axes[2].set_title(self.adc[2])

>>>>>>> live_viewer

        # Set up storage variables
        self.img_range = ()
        self.img_shape = None
<<<<<<< HEAD
        self.img_obj = None
        self.colorbar = None
        self.bar_orient = 'horizontal'

        # Render an initial empty image frame
        self.render_frame(np.zeros((10, 10), dtype=np.uint16),
                          min_val=0,
                          max_val=10)
        self.figure.tight_layout()

    def render_frame(self,
                     img_data_gn,
                     img_data_crs,
                     img_data_fn,
                     min_val=None,
                     max_val=None):
=======
        self.img_obj = {self.adc[0]: None,
                        self.adc[1]: None, 
                        self.adc[2]: None}
        self.colorbar = {self.adc[0]: None,
                         self.adc[1]: None,
                         self.adc[2]: None}
        self.bar_orient = 'horizontal'
 
        # Render an initial empty image frame
        tmp_img = (np.zeros((10, 10), dtype=np.uint16),
                   np.zeros((10, 10), dtype=np.uint16),
                   np.zeros((10, 10), dtype=np.uint16))
        self.multi_render_frame(tmp_img, min_val=0, max_val=10)
        self.figure.tight_layout()

    def multi_render_frame(self, img_data, min_val=None, max_val=None):
        """
        Redefine the number of plots.
        """
        self.render_frame(img_data[0], self.axes[0], self.adc[0])
        self.render_frame(img_data[1], self.axes[1], self.adc[1])
        self.render_frame(img_data[2], self.axes[2], self.adc[2])

    def render_frame(self, img_data, axe, adc, min_val=None, max_val=None):
>>>>>>> live_viewer
        """
        Render an image frame in the plot canvas.

        This method renders a scaled image and colorbar in the plot canvas. For speed,
        the image data is just updated if there is no change to the image shape, otherwise
        the axes and colorbar are redrawn.
        """
<<<<<<< HEAD
        # If the minimum and/or maximum values are not defined, determine from the
=======
        # If the minimum and/or maximum values are not defined, determine from the 
>>>>>>> live_viewer
        # incoming image data
        if min_val is None:
            min_val = np.amin(img_data)
        if max_val is None:
            max_val = np.amax(img_data)

        # If the shape of the incoming data has changed, delete the image object to force
        # a redraw
        if self.img_shape != img_data.shape:
            self.img_shape = img_data.shape
<<<<<<< HEAD
            self.img_obj = None
=======
            self.img_obj.update({adc: None})
>>>>>>> live_viewer

            # Set the colorbar orientation dependent on the aspect ratio of the image
            if self.img_shape[0] < self.img_shape[1]:
                self.bar_orient = 'horizontal'
            else:
                self.bar_orient = 'vertical'

            # Remove any existing colorbar prior to subequent re-draw
<<<<<<< HEAD
            if self.colorbar is not None:
                self.colorbar.remove()
                self.colorbar = None

        # If no image object exists, draw one and add a colorbar
        if self.img_obj is None:
            self.img_obj = self.axes.imshow(img_data,
                                            interpolation='nearest',
                                            vmin=min_val,
                                            vmax=max_val,
                                            cmap='jet')

            if self.colorbar is None:
                cbar_ticks = np.linspace(min_val,
                                         max_val,
                                         self.cbar_numticks,
                                         dtype=np.uint).tolist()
                self.colorbar = self.figure.colorbar(self.img_obj,
                                                     ax=self.axes,
                                                     orientation=self.bar_orient,
                                                     ticks=cbar_ticks)

        # Otherwise just update the image data for speed
        else:
            self.img_obj.set_data(img_data)
            self.axes.draw_artist(self.img_obj)
=======
            if self.colorbar[adc] is not None:
                self.colorbar[adc].remove()
                self.colorbar.update({adc: None})

        # If no image object exists, draw one and add a colorbar
        if self.img_obj[adc] is None:
            self.img_obj[adc] = axe.imshow(
                img_data, interpolation='nearest', vmin=min_val, vmax=max_val, cmap='jet')
            axe.invert_xaxis()
            if self.colorbar[adc] is None:
                cbar_ticks = np.linspace(
                    min_val, max_val, self.cbar_numticks, dtype=np.uint).tolist()
                self.colorbar[adc] = self.figure.colorbar(
                    self.img_obj[adc], ax=axe, orientation=self.bar_orient, ticks=cbar_ticks)

        # Otherwise just update the image data for speed
        else:
            self.img_obj[adc].set_data(img_data)
            axe.draw_artist(self.img_obj[adc])
>>>>>>> live_viewer

        # If the range of the data changed, update the colorbar accordingly
        if self.img_range != (min_val, max_val):
            self.img_range = (min_val, max_val)
<<<<<<< HEAD
            cbar_ticks = np.linspace(min_val,
                                     max_val,
                                     self.cbar_numticks,
                                     dtype=np.uint).tolist()
            self.colorbar.set_clim(min_val, max_val)
            self.colorbar.set_ticks(cbar_ticks)
            self.colorbar.draw_all()
=======
            cbar_ticks = np.linspace(min_val, max_val, self.cbar_numticks, dtype=np.uint).tolist()
            self.colorbar[adc].set_clim(min_val, max_val)
            self.colorbar[adc].set_ticks(cbar_ticks)
            self.colorbar[adc].draw_all()
>>>>>>> live_viewer

        # Draw the frame
        self.draw()

<<<<<<< HEAD

=======
>>>>>>> live_viewer
class MplNavigationToolbar(NavigationToolbar):
    """
    Navigation toolbar for the plot canvas.

    This class is a simple wrapper around the imported navigation toolbar.
    """
    def __init__(self, canvas, parent, coordinates=True):
        """Initialise the toolbar."""
        super(MplNavigationToolbar, self).__init__(canvas, parent, coordinates)
