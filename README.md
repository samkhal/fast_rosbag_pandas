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

## TODOs
* Add benchmarks
* Add optional support for dynamic arrays 
* Add optional support for blobs
* Allow including/excluding fields