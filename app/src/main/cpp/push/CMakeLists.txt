file(GLOB push_source *.cpp *.c)
include_directories(../../../../libs/push/include)

add_library( push
             SHARED
             ${push_source}
             )

add_library(x264
            STATIC
            IMPORTED)

set_target_properties(x264
                      PROPERTIES IMPORTED_LOCATION
                      ${DIR}/armeabi/libx264.a)


add_library(faac
            STATIC
            IMPORTED)

set_target_properties(faac
                      PROPERTIES IMPORTED_LOCATION
                      ${DIR}/armeabi/libfaac.a)

find_library(
            log-lib
            log )

target_link_libraries(push
                     ${log-lib}
                      faac
                      x264
                      rtmp
                      )