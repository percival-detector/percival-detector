"""Live view producer for odin-data.

This utility simulates the live view plugin of odin-data with random frames
to allow testing of viewer plugins and applications.

Tim Nicholls, STFC Application Engineering Group.
"""
import time
import argparse
import sys
import os

import zmq
import numpy as np


class LiveViewProducerDefaults(object):
    """
    Default parameters for the live view producer.

    This class implements a simple container of default parameters used
    for the live view producer.
    """

    def __init__(self):
        """Initialise default parameters."""
        self.endpoint_url = "tcp://127.0.0.1:5020"

        self.num_frames = 1
        self.frame_rate = 1.0

        self.image_x = 256
        self.image_y = 256
        self.image_min = 0
        self.image_max = 4096
        self.image_dtype = "uint16"


class LiveViewProducer(object):
    """
    Live view producer class.

    This class parses command line options and generated simulated live view frames of
    random data, sending them via a ZeroMQ PUB/SUB channel to viewer applicaitons or
    plugins.
    """

    def __init__(self):
        """Initialise the producer class, loading defaults and parsing command line args."""
        self.defaults = LiveViewProducerDefaults()
        self.args = self._parse_arguments()

    def _parse_arguments(self, prog_name=sys.argv[0], args=None):
        """Parse command line arguments.

        Parse command line arugments, using the default parameter object to set default
        values.
        """
        # Set the terminal width for argument help formatting
        try:
            term_columns = int(os.environ['COLUMNS']) - 2
        except (KeyError, ValueError):
            term_columns = 100

        # Build options for the argument parser
        parser = argparse.ArgumentParser(
            prog=prog_name, description='ODIN live view producer',
            formatter_class=lambda prog: argparse.ArgumentDefaultsHelpFormatter(
                prog, max_help_position=40, width=term_columns)
        )

        parser.add_argument('--frames', '-n', type=int, dest='num_frames',
                            default=self.defaults.num_frames,
                            help='Number of frames to send')
        parser.add_argument('--rate', '-r', type=float, dest='frame_rate',
                            default=self.defaults.frame_rate,
                            help='Rate to transmit frames at (Hz)')
        parser.add_argument('--endpoint', type=str, dest="endpoint_url",
                            default=self.defaults.endpoint_url,
                            help='ZeroMQ endpoint URL to bind')
        parser.add_argument('--imagex', '-x', type=int, dest='image_x',
                            default=self.defaults.image_x,
                            help='Set X dimension of image')
        parser.add_argument('--imagey', '-y', type=int, dest='image_y',
                            default=self.defaults.image_y,
                            help='Set Y dimension of image')
        parser.add_argument('--min', type=int, dest='image_min',
                            default=self.defaults.image_min,
                            help='Set minimum value of image pixel data')
        parser.add_argument('--max', type=int, dest='image_max',
                            default=self.defaults.image_max,
                            help='Set maximum value of image pixel data')
        parser.add_argument('--dtype', type=str, dest='image_dtype',
                            default=self.defaults.image_dtype,
                            help='Set the dtype of the Image, as uint8/16/32')

        parsed_args = parser.parse_args(args)
        return parsed_args

    def send_frame(self, frame_num, image_arr, flags=0, copy=True, track=False):
        """
        Send a simulated live view frame.

        This method constructs a simulated live view frame and sends it as a multipart
        message prefixed by a JSON header.
        """
        # Construct the header
        header = {
            'frame': frame_num,
            'dtype': str(image_arr.dtype),
            'dsize': image_arr.nbytes,
            'shape': image_arr.shape,
            'acquisition_id': 0,
            'dataset': 'data',
            'compression': 'none',
            'tags': ['lv_data',],
        }

        # Send the header
        self.socket.send_json(header, flags | zmq.SNDMORE)

        # Send the frame
        self.socket.send(image_arr, flags, copy=copy, track=track)

    def run(self):
        """
        Run the live view producer.

        This method creates the ZeroMQ PUB/SUB socket, binds it and then starts transmitting
        frames to subscribed viewer applications at the specified rate.
        """
        # Open the ZeroMQ SUB socket and bind it to the endpoint
        try:
            context = zmq.Context()
            self.socket = context.socket(zmq.PUB)    # pylint: disable=E1101
            self.socket.bind(self.args.endpoint_url)
        except zmq.error.ZMQError as e:
            print("Failed to connect ZeroMQ endpoint: {}".format(e))
            return 1

        # Calculate sleep time based on specified frame rate
        sleep_time = 1.0 / self.args.frame_rate

        if self.args.num_frames > 0:
            num_frames_str = str(self.args.num_frames) + ' '
        else:
            num_frames_str = ''
        print("Sending {:s}frames to endpoint {:s} at {:.1f} Hz".format(
            num_frames_str , self.args.endpoint_url, self.args.frame_rate))

        # Loop over the number of frames specified and transmit random image data with the specified
        # parameters
        frames_sent = 0
        
        # for frame in range(self.args.num_frames):
        while (self.args.num_frames == 0) or (frames_sent < self.args.num_frames):
            image_array = np.random.randint(
                self.args.image_min, self.args.image_max+1,
                (self.args.image_y, self.args.image_x), dtype=self.args.image_dtype)
            self.send_frame(frames_sent, image_array)
            frames_sent += 1
            time.sleep(sleep_time)

        return 0


def main():
    """Create and run the live view producer."""
    lvp = LiveViewProducer()
    sys.exit(lvp.run())


if __name__ == '__main__':
    main()
