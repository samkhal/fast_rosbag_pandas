#include <ros_msg_parser/ros_parser.hpp>

#include <pybind11/pybind11.h>
#include <pybind11/numpy.h>
#include <pybind11/stl.h>
#include <ros/ros.h>

#include <iostream>

namespace py = pybind11;

/// @brief Return true if the numpy and ROS representations of this type share the same byte layout
inline bool sameLayout(const RosMsgParser::BuiltinType c)
{
  using namespace RosMsgParser;
  switch (c)
  {
    case BOOL:
    case BYTE:
    case INT8:
    case CHAR:
    case UINT8:
    case UINT16:
    case INT16:
    case UINT32:
    case INT32:
    case FLOAT32:
    case UINT64:
    case INT64:
    case FLOAT64:
      return true;
    default:
      return false;
  }
}

/// @brief Get the numpy dtype name used for a ROS builtin type
inline std::string dtype_name(const RosMsgParser::BuiltinType c)
{
  using namespace RosMsgParser;
  // clang-format off
  switch (c) {
    case BOOL:    return "bool_";
    case BYTE:    return "int8";
    case INT8:    return "int8";
    case CHAR:    return "uint8";
    case UINT8:   return "uint8";    
    case UINT16:  return "uint16";
    case INT16:   return "int16"; 
    case UINT32:  return "uint32";
    case INT32:   return "int32";
    case FLOAT32: return "float32"; 
    case UINT64:  return "uint64";
    case INT64:   return "int64";
    case FLOAT64: return "float64";
    case TIME:    return "datetime64[ns]";
    case DURATION:return "timedelta64[ns]"; 
    case STRING:  return "object";
    default:
      throw std::runtime_error("Unsupported dtype");
  }
  // clang-format on
}

/// Aggregates one message field into an ndarray, one message at a time
class FieldAggregator
{
 public:
  FieldAggregator(RosMsgParser::BuiltinType type_id, size_t msg_count)
    : type_id_(type_id), dtype_size_(RosMsgParser::builtinSize(type_id))
  {
    if (type_id_ == RosMsgParser::STRING)
    {
      py_object_data_ = new std::vector<PyObject*>();
      py_object_data_->reserve(msg_count);
    }
    else
    {
      py_scalar_data_ = new std::vector<uint8_t>();
      py_scalar_data_->reserve(msg_count * dtype_size_);
    }
  }

  void addScalar(const RosMsgParser::Variant& variant)
  {
    ROS_ASSERT(type_id_ != RosMsgParser::STRING);

    if (sameLayout(type_id_))
    {
      const uint8_t* raw_data = variant.getRawStorage();
      std::copy(raw_data, raw_data + dtype_size_, std::back_inserter(*py_scalar_data_));
    }
    else if (type_id_ == RosMsgParser::TIME)
    {
      ros::Time time = variant.convert<ros::Time>();
      int64_t nsec = time.toNSec();
      const auto* raw_data = reinterpret_cast<const uint8_t*>(&nsec);
      std::copy(raw_data, raw_data + dtype_size_, std::back_inserter(*py_scalar_data_));
    }
    else if (type_id_ == RosMsgParser::DURATION)
    {
      ros::Duration dur = variant.convert<ros::Duration>();
      int64_t nsec = dur.toNSec();
      const auto* raw_data = reinterpret_cast<const uint8_t*>(&nsec);
      std::copy(raw_data, raw_data + dtype_size_, std::back_inserter(*py_scalar_data_));
    }
    else
      throw std::runtime_error("Attempted to add invalid scalar to FieldAggregator");
  }

  void addString(const std::string& string)
  {
    ROS_ASSERT(type_id_ == RosMsgParser::STRING);

    PyObject* obj = py::str(string).release().ptr();
    py_object_data_->push_back(obj);
  }

  py::array getPyArray()
  {
    py::dtype dtype(dtype_name(type_id_));

    if (type_id_ == RosMsgParser::STRING)
    {
      if (py_object_data_->empty())
        throw std::runtime_error("No data found");

      py::capsule free_when_done(py_object_data_, [](void* f) {
        auto* object_vector = reinterpret_cast<std::vector<PyObject*>*>(f);
        // Decrement the reference count for PyObjects we created
        for (PyObject* ptr : *object_vector)
          py::handle(ptr).dec_ref();

        delete object_vector;
      });

      return py::array(dtype, py_object_data_->size(), py_object_data_->data());
    }
    else
    {
      if (py_scalar_data_->empty())
        throw std::runtime_error("No data found");

      //   Free the underlying array when the ndarray is destroyed
      py::capsule free_when_done(py_scalar_data_, [](void* f) {
        auto* foo = reinterpret_cast<std::vector<uint8_t>*>(f);
        delete foo;
      });

      return py::array(dtype, py_scalar_data_->size() / dtype_size_, py_scalar_data_->data(), free_when_done);
    }
  }

 private:
  RosMsgParser::BuiltinType type_id_;
  int dtype_size_ = 0;  // Only valid for fixed-size, non-string type_id_

  std::vector<uint8_t>* py_scalar_data_ = nullptr;
  std::vector<PyObject*>* py_object_data_ = nullptr;
};

/// Aggregates one topic into a set of ndarrays, one per field
class TopicAggregator
{
 public:
  TopicAggregator(const rosbag::ConnectionInfo& connection, size_t msg_count)
    : topic_(connection.topic)
    , parser_(connection.topic, connection.datatype, connection.msg_def)
    , msg_count_(msg_count)
  {
  }

  void addMessage(const rosbag::MessageInstance& msg)
  {
    // write the message into the buffer
    buffer_.resize(msg.size());
    ros::serialization::OStream stream(buffer_.data(), msg.size());
    msg.write(stream);

    RosMsgParser::Span<const uint8_t> buffer(buffer_.data(), msg.size());
    parser_.deserializeIntoFlatMsg(buffer, &flat_msg_);

    for (auto& [leaf, variant] : flat_msg_.value)
    {
      bool valid_aggregator = initFieldAggregatorIfNeeded(leaf, variant.getTypeID(), msg_count_);

      if (valid_aggregator)
        field_aggregators_.at(leaf.toStdString()).addScalar(variant);
    }
    for (auto& [leaf, string] : flat_msg_.name)
    {
      bool valid_aggregator = initFieldAggregatorIfNeeded(leaf, RosMsgParser::STRING, msg_count_);

      if (valid_aggregator)
        field_aggregators_.at(leaf.toStdString()).addString(string);
    }

    fields_initialized_ = true;
  }

  /// Collect ndarrays. Returns mapping of field paths to ndarrays
  std::unordered_map<std::string, py::array> getPyArrays()
  {
    std::unordered_map<std::string, py::array> arrays;
    for (auto& [name, field_aggregator] : field_aggregators_)
    {
      // Strip topic name from the beginning of the leaf path
      std::string topic_prefix = topic_ + "/";
      auto startswith = [](auto& str, auto& prefix) { return str.rfind(prefix, 0) == 0; };
      if (startswith(name, topic_prefix))
        arrays[name.substr(topic_prefix.size())] = field_aggregator.getPyArray();
    }
    return arrays;
  }

 private:
  bool initFieldAggregatorIfNeeded(const RosMsgParser::FieldTreeLeaf& leaf,
                                   RosMsgParser::BuiltinType type,
                                   size_t msg_count)
  {
    // Supporting dynamic arrays means any field could be nullable; don't support for now
    if (isInDynamicArray(leaf))
      return false;

    if (!fields_initialized_)
      field_aggregators_.emplace(std::make_pair(leaf.toStdString(), FieldAggregator(type, msg_count)));

    return true;
  }

  /// Check if any of the parents of this node is a variable-length array
  bool isInDynamicArray(const RosMsgParser::FieldTreeLeaf& leaf)
  {
    return isInDynamicArray(*leaf.node_ptr);
  }

  bool isInDynamicArray(const RosMsgParser::FieldTreeNode& node)
  {
    if (node.value()->arraySize() == -1)
      return true;
    else if (node.parent() == nullptr)
      return false;
    else
      return isInDynamicArray(*node.parent());
  }

  std::string topic_;
  size_t msg_count_;
  RosMsgParser::Parser parser_;

  bool fields_initialized_ = false;
  std::unordered_map<std::string, FieldAggregator> field_aggregators_;

  // Buffers
  RosMsgParser::FlatMessage flat_msg_;
  std::vector<uint8_t> buffer_;
};

/**
 * Given a path to a bag and a topic, extract that topic into ndarrays, one array per field.
 * Arrays are keyed on the field's path within the message, e.g. "foo/bar.1/baz"
 */
std::unordered_map<std::string, py::array> rosbag_to_ndarrays(const std::string& bag_path, const std::string& topic)
{
  rosbag::Bag bag;
  bag.open(bag_path);

  rosbag::View bag_view(bag, rosbag::TopicQuery({topic}));

  std::unique_ptr<TopicAggregator> topic_aggregator;
  for (const rosbag::ConnectionInfo* connection : bag_view.getConnections())
  {
    if (connection->topic == topic)
    {
      topic_aggregator = std::make_unique<TopicAggregator>(*connection, bag_view.size());
      break;
    }
  }
  if (!topic_aggregator)
    throw std::runtime_error(std::string("Topic \"") + topic + "\" not found in bag");

  for (rosbag::MessageInstance msg_instance : bag_view)
    topic_aggregator->addMessage(msg_instance);

  return topic_aggregator->getPyArrays();
}

PYBIND11_MODULE(rosbag_to_ndarrays, m)
{
  m.def("rosbag_to_ndarrays", &rosbag_to_ndarrays);
}