if( MSVC )
	set( EXTRA_FLAGS "/Oi /Zm250 /D_CRT_SECURE_NO_WARNINGS" )

    set( CMAKE_CXX_FLAGS_DEBUG_INIT          "/MDd /Od /Ob0 /D_DEBUG /Zi /RTC1   ${EXTRA_FLAGS}" )
    set( CMAKE_CXX_FLAGS_MINSIZEREL_INIT     "/MD  /O1 /Ob1 /DNDEBUG             ${EXTRA_FLAGS}" )
    set( CMAKE_CXX_FLAGS_RELEASE_INIT        "/MD  /O2 /Ob2 /DNDEBUG /Zi /arch:SSE2  ${EXTRA_FLAGS}" )
    set( CMAKE_CXX_FLAGS_RELWITHDEBINFO_INIT "/MD  /O2 /Ob1 /DNDEBUG /Zi         ${EXTRA_FLAGS}" )

    set( CMAKE_EXE_LINKER_FLAGS_RELEASE "/debug /INCREMENTAL:NO" )
else()
	set( EXTRA_FLAGS "-std=c++11 -mfpmath=sse -msse -msse2 -msse3 -mssse3 -Wno-literal-suffix -Wdeprecated-declarations" )

    set( CMAKE_CXX_FLAGS_INIT                  "-O3 -DNDEBUG    ${EXTRA_FLAGS}" )
	set( CMAKE_CXX_FLAGS_DEBUG_INIT            "-O0 -g -D_DEBUG -fno-inline ${EXTRA_FLAGS}" )
    set( CMAKE_CXX_FLAGS_MINSIZEREL_INIT       "-Os -DNDEBUG    ${EXTRA_FLAGS}" )
    set( CMAKE_CXX_FLAGS_RELEASE_INIT          "-O3 -DNDEBUG    ${EXTRA_FLAGS}" )
    set( CMAKE_CXX_FLAGS_RELWITHDEBINFO_INIT   "-O2 -g -DNDEBUG ${EXTRA_FLAGS}" )
endif()
