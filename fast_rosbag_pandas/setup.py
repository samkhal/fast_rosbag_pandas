from distutils.core import setup

from catkin_pkg.python_setup import generate_distutils_setup

d = generate_distutils_setup(
    packages=["fast_rosbag_pandas"],
    scripts=[],
    package_dir={"": "src"},
    keywords=["ROS", "rosbag", "pandas"],
)

setup(**d)