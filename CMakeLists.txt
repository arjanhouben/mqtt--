if( PROJECT )
	set( SUBPROJECT TRUE )
endif()

project( mqttpp )

cmake_minimum_required( VERSION 3.19 )

include( FetchContent )

option( WITH_CLIENTS OFF )
option( WITH_BROKER OFF )
option( WITH_APPS OFF )
option( WITH_PLUGINS OFF )
option( DOCUMENTATION OFF )

FetchContent_Declare( mosquitto 
	GIT_REPOSITORY
		https://github.com/eclipse/mosquitto.git
)

FetchContent_Makeavailable( mosquitto )

add_library( mqttpp INTERFACE )

target_compile_features( mqttpp
	INTERFACE
		cxx_std_20
)

target_include_directories( mqttpp
	INTERFACE
		include
		${mosquitto_SOURCE_DIR}/include
)

target_link_libraries( mqttpp
	INTERFACE
		libmosquitto
)

if ( NOT SUBPROJECT )

	add_executable( mqttpp_example
		main.cpp
	)

	target_compile_options( mqttpp_example
		PRIVATE
			-Wall
			-Wpedantic
			-Wconversion
	)

	target_link_libraries( mqttpp_example
		PRIVATE
			mqttpp
	)

endif()