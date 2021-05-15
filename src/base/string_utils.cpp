#include "string_utils.hpp"

#include <algorithm>
#include <cassert>

namespace rigel::strings
{

std::vector<std::string> split(std::string_view input, char delimiter)
{
  assert(
    static_cast<int>(delimiter) < 127 && "We only accept ASCII delimiters");

  std::vector<std::string> output;
  auto start = std::begin(input);
  auto end = std::end(input);
  decltype(start) next;
  while ((next = std::find(start, end, delimiter)) != end)
  {
    output.emplace_back(start, next);
    start = std::next(next, 1);
  }
  output.emplace_back(start, next);
  return output;
}

bool startsWith(std::string_view input, std::string_view prefix) noexcept
{
  return input.size() >= prefix.size() &&
    std::equal(std::begin(prefix), std::end(prefix), std::begin(input));
}

std::string& trimLeft(std::string& input, const char* what) noexcept
{
  input.erase(0, input.find_first_not_of(what));
  return input;
}

std::string& trimRight(std::string& input, const char* what) noexcept
{
  input.erase(input.find_last_not_of(what) + 1);
  return input;
}

std::string& trim(std::string& input, const char* what) noexcept
{
  trimLeft(trimRight(input, what), what);
  return input;
}

} // namespace rigel::strings
