#include <fast_rosbag_pandas/rosbag_to_ndarrays.h>

#include <string>

int main(int argc, char* argv[])
{
  if (argc != 3)
  {
    printf("Usage: benchmark [bag file] [topic]");
    return -1;
  }

  std::string bag_path(argv[1]);
  std::string topic(argv[2]);
  // Avoid construction of py-objects that fails when this is run outside of python
  bool discard_return = true;

  fast_rosbag_pandas::rosbag_to_ndarrays(bag_path, topic, discard_return);
  return 0;
}