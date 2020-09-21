# fast_rosbag_pandas

A high-speed library for parsing rosbags into Pandas DataFrames. C++ under the hood makes it much faster than approaches using the Python rosbag API.

Example:
```py
>>> from fast_rosbag_pandas import rosbag_to_dataframe
>>> print(rosbag_to_dataframe("example.bag", "stamped_point_topic"))

  header/frame_id  header/seq        header/stamp  point/x  point/y  point/z
0          frame1           0 2020-01-01 00:01:40      1.0      2.0      3.0
1          frame2           1 2020-01-01 00:01:40      4.0      5.0      6.0
```

Conversion rules are as follows:
  * One column is produced per field of the ROS message
  * Column names are the path to the field, e.g. pose/point/x
  * Column dtypes:
    * ROS `string` becomes numpy `object`
    * ROS `Time`/`Duration` become numpy `datetime64[ns]`/`timedelta64[ns]`
    * All other primitive ROS types are directly mapped to numpy primitives
  * Fixed-size arrays are unpacked into separate columns, with names like `foo/bar.2/baz`
  * Dynamic-size arrays are skipped

# Benchmarks

Benchmarking code is at [fast_rosbag_pandas_benchmark](https://github.com/samkhal/fast_rosbag_pandas_benchmark).

Graphs of performance over time for this package are on the [airspeed velocity site](https://samkhal.github.io/fast_rosbag_pandas_benchmark).

Benchmarks comparing `fast_rosbag_pandas` (latest) to `rosbag_pandas` (0.5.0.0, pure python implementation) are below.
Note: fast_rosbag_pandas doesn't currently construct an index, while rosbag_pandas does.

## Bag of StampedPoint messages
|                |   rosbag_pandas (s) |   fast_rosbag_pandas (s) |   speedup factor |
|:---------------|--------------------:|-------------------------:|-----------------:|
| points_1k.bag  |              0.3419 |                   0.0025 |            137.4 |
| points_1m.bag  |             20.6032 |                   1.1907 |             17.3 |
| points_10m.bag |            197.8608 |                  11.6739 |             16.9 |

# Limitations
This library assumes it is built and run on little-endian architectures only.

Currently, fast_rosbag_pandas does not support:
* Building an index
* Dynamic-length arrays

# TODOs
* Expose blob/array handling options
* Add optional support for dynamic arrays 
* Add optional support for blobs
* Allow including/excluding fields
* Support extracting multiple topics at once