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

# Benchmarks

fast_rosbag_pandas is about 3x faster than rosbag_pandas on large datasets, and ~30x faster on small ones.
Note: fast_rosbag_pandas doesn't currently construct an index, while rosbag_pandas does.

## Bag of StampedPoint messages
|                |   rosbag_pandas (s)   |fast_rosbag_pandas (s)|
|:---------------|----------------------:|---------------------:|
| points_1k.bag  |              0.316    |           0.009      |
| points_1m.bag  |             19.707    |           6.980      |
| points_10m.bag |            196.645    |          69.321      |


# Caveats
This library assumes it is built and run on little-endian architectures only.

# TODOs
* Profile
* Expose blob/array handling options
* Add optional support for dynamic arrays 
* Add optional support for blobs
* Allow including/excluding fields
* Support extracting multiple topics at once