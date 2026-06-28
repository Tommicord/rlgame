export module Rl.RayLog.FormatParser;

import <string>;
import <vector>;
import <sstream>;

namespace Rl::RayLog
{

export class RayLogFormatParser
{
  public:
  struct FormatToken
  {
    bool isSpecifier;
    std::string value;
  };

  [[nodiscard]]
  static std::vector<FormatToken> Parse(const std::string& format)
  {
    std::vector<FormatToken> tokens;
    std::string current;

    for (size_t i = 0; i < format.size(); ++i)
    {
      if (format[i] == '%' && i + 1 < format.size())
      {
        if (!current.empty())
        {
          tokens.push_back({false, current});
          current.clear();
        }
        char spec = format[i + 1];
        if (spec == 's' || spec == 'd' || spec == 'f' || spec == 'h' || spec == 'p' || spec == 'b' || spec == 'a')
        {
          tokens.push_back({true, std::string(1, spec)});
          i++;
        }
        else if (spec == 'f' && i + 2 < format.size() && format[i + 2] == '.')
        {
          std::string precision;
          size_t j = i + 3;
          while (j < format.size() && std::isdigit(format[j]))
          {
            precision += format[j];
            j++;
          }
          tokens.push_back({true, "%f." + precision});
          i = j - 1;
        }
        else
        {
          current += format[i];
        }
      }
      else
      {
        current += format[i];
      }
    }
    if (!current.empty())
      tokens.push_back({false, current});
    return tokens;
  }
};

}
