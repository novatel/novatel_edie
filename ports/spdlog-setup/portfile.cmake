vcpkg_from_github(
    OUT_SOURCE_PATH SOURCE_PATH
    REPO gegles/spdlog_setup
    REF "v${VERSION}"
    SHA512 da580028cfcffe2112b6063190b0b7bf1059c9137a7a23657d88b002995e8c901fa87d2e0bb15333632e4547b318bc593e7764c9e2e4b5a1ad371a342a0c3fb2
    HEAD_REF main
)

file(INSTALL "${SOURCE_PATH}/include/spdlog_setup" DESTINATION "${CURRENT_PACKAGES_DIR}/include")
file(INSTALL "${CMAKE_CURRENT_LIST_DIR}/spdlog_setup-config.cmake" DESTINATION "${CURRENT_PACKAGES_DIR}/share/spdlog_setup")

file(INSTALL "${CMAKE_CURRENT_LIST_DIR}/usage" DESTINATION "${CURRENT_PACKAGES_DIR}/share/${PORT}")
vcpkg_install_copyright(FILE_LIST "${SOURCE_PATH}/LICENSE")
