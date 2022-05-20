# enable packing
set(CPACK_PROJECT_NAME ${PROJECT_NAME})
set(CPACK_PACKAGE_VENDOR "Nikolai Wuttke <lethal_guitar128@web.de>")
set(CPACK_PACKAGE_CONTACT "Nikolai Wuttke <lethal_guitar128@web.de>")
set(CPACK_PACKAGE_DESCRIPTION [[
RigelEngine is a re-implementation of the game Duke Nukem II, originally
released by Apogee Software in 1993 for MS-DOS.  It works as a drop-in
replacement for the original executable: It reads the game's data files and
plays just like the original, but runs natively on modern operating systems.
On top of that, it offers various modern enhancements like higher frame rate,
better game controller support, a wide-screen mode, quick saving etc.

In order to run RigelEngine, the game data from the original game is required.
Both the shareware version and the registered version work.

You can grab the shareware version from archive.org:
https://archive.org/download/msdos_DUKE2_shareware/DUKE2.zip
]]
)
set(CPACK_PACKAGE_DESCRIPTION_SUMMARY "A modern re-implementation of the game Duke Nukem II")
set(CPACK_PACKAGE_HOMEPAGE_URL "https://github.com/lethal-guitar/RigelEngine")
set(CPACK_PACKAGE_CHECKSUM "SHA256")
set(CPACK_PACKAGE_INSTALL_DIRECTORY "RigelEngine")
set(CPACK_PACKAGE_VERSION_MAJOR ${PROJECT_VERSION_MAJOR})
set(CPACK_PACKAGE_VERSION_MINOR ${PROJECT_VERSION_MINOR})
set(CPACK_PACKAGE_VERSION_PATCH ${PROJECT_VERSION_PATCH})
set(CPACK_PACKAGE_EXECUTABLES "RigelEngine")
set(CPACK_CREATE_DESKTOP_LINKS "RigelEngine")
set(CPACK_OUTPUT_FILE_PREFIX packages)
set(CPACK_STRIP_FILES TRUE)
set(CPACK_ARCHIVE_COMPONENT_INSTALL ON)
set(CPACK_COMPONENTS_ALL bin)
set(CPACK_PACKAGE_FILE_NAME "rigelengine")

# We don't include this in order to avoid popping up a "Agree to EULA" dialog
# for some CPack generators, like Mac OS
#set(CPACK_RESOURCE_FILE_LICENSE "${PROJECT_SOURCE_DIR}/LICENSE.md")
set(CPACK_RESOURCE_FILE_README "${PROJECT_SOURCE_DIR}/README.md")

# a headsup
set(CPACK_WARN_ON_ABSOLUTE_INSTALL_DESTINATION TRUE)

# Debian
# (abencsik) intentionally left this here for future reference
# set(CPACK_DEBIAN_PACKAGE_SHLIBDEPS ON) # automatically resolve deps
set(CPACK_DEBIAN_PACKAGE_DEPENDS "libc6 (>= 2.27), libstdc++6 (>= 8.4.0), libsdl2-2.0-0 (>= 2.0.4), libsdl2-mixer-2.0-0 (>= 2.0.1)")
set(CPACK_DEBIAN_PACKAGE_SECTION "games")
set(CPACK_DEBIAN_PACKAGE_RECOMMENDS "")
set(CPACK_DEBIAN_PACKAGE_SUGGESTS "")

include(CPack)
