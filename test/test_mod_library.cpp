#include <base/warnings.hpp>
#include <data/mod_library.hpp>

RIGEL_DISABLE_WARNINGS
#include <catch.hpp>
RIGEL_RESTORE_WARNINGS

#include <cstdio>

TEST_CASE("Mod library rescan")
{
  namespace fs = std::filesystem;

  rigel::data::ModLibrary ml{"/tmp", {}, {}};

  Filesystem mockFsHandle{
    [](const std::filesystem::path& p, std::error_code& ec) noexcept -> bool
    { return true; },
    [](const std::filesystem::path& p, std::error_code& ec) noexcept -> bool
    { return true; },
    [](
      const std::filesystem::path& p,
      std::filesystem::directory_options options,
      std::error_code& ec,
      Filesystem::DirectoryIterateCallbackFuncT callback,
      void* user_data) noexcept {
    }};

  ml.rescan(mockFsHandle);
}
