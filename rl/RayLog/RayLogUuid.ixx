export module Rl.RayLog.Uuid;

import Rl.RayLog.IRayLogSerializable;

import <random>;
import <sstream>;
import <iomanip>;
import <array>;
import <chrono>;

namespace Rl::RayLog
{

/* RayLog logging thread UUID */
export class RayLogUuid final : IRayLogSerializable<void>
{
  /* The UUID bytes data */
  std::array<unsigned char, 16> data{};

  public:
  /* Constructs a UUID for RayLog logging thread */
  RayLogUuid()
  {
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<unsigned char> dis(0, 255);

    for (auto& byte : data)
      byte = dis(gen);

    data[6] = (data[6] & 0x0F) | 0x40;
    data[8] = (data[8] & 0x3F) | 0x80;
  }

  /* Returns the formatted UUID string */
  [[nodiscard]]
  constexpr const std::string& ToString() const override
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
}
