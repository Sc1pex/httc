add_library(httc)

target_compile_features(httc PUBLIC cxx_std_23)

target_link_libraries(httc PUBLIC asio)

target_sources(httc
    PRIVATE
        ./headers.cpp
        ./io.cpp
        ./percent_encoding.cpp
        ./request.cpp
        ./request_parser.cpp
        ./response.cpp
        ./router.cpp
        ./server.cpp
        ./uri.cpp
        ./validation.cpp
        ./utils/mime.cpp
        ./utils/fs.cpp

    PUBLIC
        FILE_SET HEADERS
        BASE_DIRS
            ${PROJECT_SOURCE_DIR}/include
        FILES
            ${PROJECT_SOURCE_DIR}/include/httc/headers.hpp
            ${PROJECT_SOURCE_DIR}/include/httc/io.hpp
            ${PROJECT_SOURCE_DIR}/include/httc/percent_encoding.hpp
            ${PROJECT_SOURCE_DIR}/include/httc/request.hpp
            ${PROJECT_SOURCE_DIR}/include/httc/request_parser.hpp
            ${PROJECT_SOURCE_DIR}/include/httc/response.hpp
            ${PROJECT_SOURCE_DIR}/include/httc/router.hpp
            ${PROJECT_SOURCE_DIR}/include/httc/server.hpp
            ${PROJECT_SOURCE_DIR}/include/httc/server_config.hpp
            ${PROJECT_SOURCE_DIR}/include/httc/status.hpp
            ${PROJECT_SOURCE_DIR}/include/httc/uri.hpp
            ${PROJECT_SOURCE_DIR}/include/httc/validation.hpp
            ${PROJECT_SOURCE_DIR}/include/httc/utils/mime.hpp
            ${PROJECT_SOURCE_DIR}/include/httc/utils/fs.hpp
)
