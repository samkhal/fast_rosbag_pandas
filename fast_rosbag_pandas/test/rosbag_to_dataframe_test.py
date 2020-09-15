from fast_rosbag_pandas import rosbag_to_dataframe

import numpy as np
import pandas as pd

import rosbag
import rospy
import time
import sys
import os
import unittest
import tempfile
from contextlib import closing

from std_msgs.msg import (
    Int8,
    Int16,
    Int32,
    Int64,
    UInt8,
    UInt16,
    UInt32,
    UInt64,
    Float32,
    Float64,
    String,
    Bool,
    Time,
    Duration,
    Header,
    MultiArrayLayout,
    MultiArrayDimension,
)

from geometry_msgs.msg import PointStamped, Point
from sensor_msgs.msg import Imu

STD_INT_MSG_TYPES = [Int8, Int16, Int32, Int64, UInt8, UInt16, UInt32, UInt64]
STD_FLOAT_MSG_TYPES = [Float32, Float64]


class TestTemp(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        _, cls.bagpath = tempfile.mkstemp()

        with closing(rosbag.Bag(cls.bagpath, "w")) as bag:
            for msg_type in STD_INT_MSG_TYPES:
                bag.write("topic_" + msg_type.__name__, msg_type(5))
                bag.write("topic_" + msg_type.__name__, msg_type(10))

            for msg_type in STD_FLOAT_MSG_TYPES:
                bag.write("topic_" + msg_type.__name__, msg_type(2.5))
                bag.write("topic_" + msg_type.__name__, msg_type(2.6))

            bag.write("topic_bool", Bool(True))
            bag.write("topic_bool", Bool(False))

            bag.write("topic_time", Time(rospy.Time(100)))
            bag.write("topic_time", Time(rospy.Time(200)))

            bag.write("topic_duration", Duration(rospy.Duration(100)))
            bag.write("topic_duration", Duration(rospy.Duration(200)))

            bag.write("topic_string", String("mystring1"))
            bag.write("topic_string", String("mystring2"))

            bag.write("topic_pstamped", PointStamped(Header(0, rospy.Time(100), "frame1"), Point(1, 2, 3)))
            bag.write("topic_pstamped", PointStamped(Header(0, rospy.Time(100), "frame2"), Point(4, 5, 6)))

            imu_msg = Imu()
            imu_msg.orientation_covariance[:3] = [1, 2, 3]
            bag.write("topic_imu", imu_msg)

            multiarray_msg = MultiArrayLayout()
            multiarray_msg.dim = [MultiArrayDimension("", 1, 2)]
            multiarray_msg.data_offset = 5
            bag.write("topic_multiarraylayout", multiarray_msg)

    @classmethod
    def tearDownClass(cls):
        os.remove(cls.bagpath)

    def test_integer(self):
        for msg_type in STD_INT_MSG_TYPES:
            df = rosbag_to_dataframe(self.bagpath, "topic_" + msg_type.__name__)
            self.assertEqual(str(df.dtypes["data"]), msg_type.__name__.lower())
            self.assertEqual(len(df["data"]), 2)
            self.assertEqual(df["data"][0], 5)
            self.assertEqual(df["data"][1], 10)

    def test_floats(self):
        for msg_type in STD_FLOAT_MSG_TYPES:
            df = rosbag_to_dataframe(self.bagpath, "topic_" + msg_type.__name__)
            self.assertEqual(str(df.dtypes["data"]), msg_type.__name__.lower())
            self.assertEqual(len(df["data"]), 2)
            self.assertAlmostEqual(df["data"][0], 2.5, places=5)
            self.assertAlmostEqual(df["data"][1], 2.6, places=5)

    def test_bool(self):
        df = rosbag_to_dataframe(self.bagpath, "topic_bool")
        self.assertEqual(str(df.dtypes["data"]), "bool")
        self.assertEqual(len(df["data"]), 2)
        self.assertEqual(df["data"][0], True)
        self.assertEqual(df["data"][1], False)

    def test_time(self):
        df = rosbag_to_dataframe(self.bagpath, "topic_time")
        self.assertEqual(str(df.dtypes["data"]), "datetime64[ns]")
        self.assertEqual(len(df["data"]), 2)
        self.assertEqual(df["data"][0], np.datetime64(int(100e9), "ns"))
        self.assertEqual(df["data"][1], np.datetime64(int(200e9), "ns"))

    def test_duration(self):
        df = rosbag_to_dataframe(self.bagpath, "topic_duration")
        self.assertEqual(str(df.dtypes["data"]), "timedelta64[ns]")
        self.assertEqual(len(df["data"]), 2)
        self.assertEqual(df["data"][0], np.timedelta64(int(100e9), "ns"))
        self.assertEqual(df["data"][1], np.timedelta64(int(200e9), "ns"))

    def test_string(self):
        df = rosbag_to_dataframe(self.bagpath, "topic_string")
        self.assertEqual(str(df.dtypes["data"]), "object")
        self.assertEqual(len(df["data"]), 2)
        self.assertEqual(df["data"][0], "mystring1")
        self.assertEqual(df["data"][1], "mystring2")

    def test_tree(self):
        df = rosbag_to_dataframe(self.bagpath, "topic_pstamped")
        self.assertEqual(len(df), 2)
        self.assertEqual(df["point/y"][0], 2)
        self.assertEqual(df["point/y"][1], 5)
        self.assertEqual(df["header/frame_id"][0], "frame1")
        self.assertEqual(df["header/frame_id"][1], "frame2")

    def test_fixed_array(self):
        df = rosbag_to_dataframe(self.bagpath, "topic_imu")
        self.assertEqual(df["orientation_covariance.0"][0], 1)
        self.assertEqual(df["orientation_covariance.1"][0], 2)

    def test_variable_array(self):
        df = rosbag_to_dataframe(self.bagpath, "topic_multiarraylayout")
        self.assertEqual(df["data_offset"][0], 5)  # Scalar fields are present
        self.assertEqual(len(df.columns), 1)  # No columns exist for variable-length array

    def test_missing_topic(self):
        with self.assertRaisesRegexp(RuntimeError, 'Topic "invalid_topic" not found in bag'):
            rosbag_to_dataframe(self.bagpath, "invalid_topic")

    def test_invalid_bag(self):
        with self.assertRaises(RuntimeError):
            rosbag_to_dataframe("nonsense_path", "invalid_topic")


if __name__ == "__main__":
    unittest.main()