export module Rl.RayLog.Uuid;

import Rl.RayLog.ISerializable;

import <random>;
import <sstream>;
import <iomanip>;
import <array>;
import <chrono>;

namespace Rl::RayLog
{

/* RayLog logging thread UUID */
export class RayLogUuid final : IRayLogSerializable2
{
  /* The UUID bytes data */
  std::array<unsigned char, 16> data{};

  public:
  /* Constructs a UUID for RayLog logging thread */
  RayLogUuid()
  {
    std::random_device                          rd;
    std::mt19937                                gen(rd());
    std::uniform_int_distribution<unsigned int> dis(0, 255);

    for (auto& byte : data)
      byte = static_cast<unsigned char>(dis(gen));

    data[6] = (data[6] & 0x0F) | 0x40;
    data[8] = (data[8] & 0x3F) | 0x80;
  }

  /* Returns the formatted UUID string */
  [[nodiscard]]
  std::string ToString() const
  {
    std::stringstream ss;
    for (size_t i = 0; i < data.size(); ++i)
    {
      ss << std::hex << std::setw(2) << std::setfill('0') << static_cast<int>(data[i]);
      if (i == 3 || i == 5 || i == 7 || i == 9)
        ss << '-';
    }
    return ss.str();
  }
};
} // namespace Rl::RayLog
