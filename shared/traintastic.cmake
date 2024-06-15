# Version
set(TRAINTASTIC_VERSION 0.3.0)
set(TRAINTASTIC_CODENAME "master")

# Platform
if(UNIX AND NOT APPLE AND NOT MINGW)
  set(LINUX TRUE)
endif()

# CI/CD or local
if(DEFINED ENV{GITHUB_ACTIONS})
  message(STATUS "We're on GitHub Actions")

  if($ENV{GITHUB_REF_TYPE} MATCHES "branch")
    set(TRAINTASTIC_VERSION_EXTRA "-$ENV{CI_REF_NAME_SLUG}-$ENV{GITHUB_RUN_NUMBER}-$ENV{CI_SHA_SHORT}")
  endif()
else() # Local
  set(TRAINTASTIC_VERSION_EXTRA "-dev")
endif()

if(DEFINED TRAINTASTIC_VERSION_EXTRA)
  string(REGEX REPLACE "^-" "" TRAINTASTIC_VERSION_EXTRA_NODASH ${TRAINTASTIC_VERSION_EXTRA})
endif()

# Debian package
if(LINUX)
  cmake_host_system_information(RESULT DISTRO QUERY DISTRIB_ID_LIKE)
  cmake_host_system_information(RESULT CODENAME QUERY DISTRIB_VERSION_CODENAME)

  message(STATUS "distro id is ${DISTRO}")
  message(STATUS "release codename is ${CODENAME}")

  string(REPLACE "-" "~" DEBIAN_PACKAGE_VERSION_EXTRA ~${DISTRO}~${CODENAME}${TRAINTASTIC_VERSION_EXTRA})
endif()

# Debug
message(STATUS "TRAINTASTIC_VERSION_EXTRA=${TRAINTASTIC_VERSION_EXTRA}")
message(STATUS "DEBIAN_PACKAGE_VERSION_EXTRA=${DEBIAN_PACKAGE_VERSION_EXTRA}")
