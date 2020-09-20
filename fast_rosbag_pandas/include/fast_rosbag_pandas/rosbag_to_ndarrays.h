#include <pybind11/numpy.h>

#include <unordered_map>
#include <string>

namespace fast_rosbag_pandas
{
/**
 * Given a path to a bag and a topic, extract that topic into ndarrays, one array per field.
 * Arrays are keyed on the field's path within the message, e.g. "foo/bar.1/baz"
 *
 * @param bag_path path to the bag to load
 * @param topic topic name to extract from the bag
 * @param discard_return_value For testing only (allows benchmarking in a non-python context)
 */
PYBIND11_EXPORT std::unordered_map<std::string, pybind11::array> rosbag_to_ndarrays(const std::string& bag_path,
                                                                                    const std::string& topic,
                                                                                    bool discard_return_value = false);
}  // namespace fast_rosbag_pandas