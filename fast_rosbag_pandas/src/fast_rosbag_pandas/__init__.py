"""fast_rosbag_pandas

A fast converter of rostopics to pandas dataframes
"""

import rosbag_to_ndarrays
import pandas


def rosbag_to_dataframe(bagpath, topic):
    """Get a dataframe for a given topic in a rosbag

    Conversion rules are as follows:
     * One column is produced per field of the ROS message
     * Column names are the path to the field, e.g. pose/point/x
     * Column dtypes:
       * ROS string becomes object
       * ROS time/duration become datetime64[ns]/timedelta64[ns]
       * Primitive types are translated directly
     * Fixed-size arrays are unpacked, e.g. foo/bar.2/baz
     * Dynamic-size arrays are ignored

    Args:
        bagpath (str): path to rosbag
        topic (str): name of topic to extract from the bag

    Returns:
        pandas.DataFrame: dataframe representation of topic
    """

    array = rosbag_to_ndarrays.rosbag_to_ndarrays(bagpath, topic)
    return pandas.DataFrame(array)
