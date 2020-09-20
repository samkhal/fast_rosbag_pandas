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

  fast_rosbag_pandas::rosbag_to_ndarrays(bag_path, topic);
  return 0;
}