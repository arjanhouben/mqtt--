#pragma once

#include <string>
#include <chrono>
#include <any>
#include <vector>
#include <span>
#include <mosquitto.h>

namespace arjan {
namespace mqttpp {

namespace helper
{

struct mosquitto_ptr
{
	template < typename T >
	void operator()( T t ) noexcept
	{
		mosquitto_disconnect( t );
	}
};

inline void throw_errno()
{
	throw std::system_error( errno, std::generic_category() );
}

int handle_error( int result )
{
	if ( MOSQ_ERR_SUCCESS != result )
	{
		throw_errno();
	}
	return result;
}

template < typename T >
T* handle_error( T *result )
{
	if ( !result )
	{
		throw_errno();
	}
	return result;
}

}

struct init
{
	init() noexcept { mosquitto_lib_init(); }
	~init() { mosquitto_lib_cleanup(); }
};

using mosquitto_ptr = std::unique_ptr< mosquitto, helper::mosquitto_ptr >;

enum class retain
{
	yes,
	no
};

struct subscription_callback
{
	using callback_type = void (*)( const mosquitto_message&, std::any& );

	template < typename Callback >
	explicit subscription_callback( Callback &&callback ) :
		storage_( std::move( callback ) ),
		action_( 
			[]( const mosquitto_message &m, std::any &any )
			{
				std::any_cast< Callback >( any )( m );
			}
		)
	{
	}
	void call( const mosquitto_message *m )
	{
		action_( *m, storage_ );
	}

	private:
		std::any storage_;
		const callback_type action_;
};

struct host : init
{
	std::string host = "127.0.0.1";
	int port = 1883;
	std::chrono::seconds keep_alive_interval = std::chrono::seconds( 60 );
};

struct publisher
{
	explicit publisher( const host &h ) :
		mosquitto_( helper::handle_error( mosquitto_new( nullptr, true, this ) ) )
	{
		helper::handle_error(
			mosquitto_connect(
				mosquitto_.get(), 
				h.host.c_str(), 
				h.port, 
				static_cast< int >( h.keep_alive_interval.count() )
			)
		);
	}

	template < typename T, size_t N >
	void publish( const std::string &topic, std::span< T, N > data, retain retain_message = retain::no )
	{
		auto bytes = std::as_bytes( data );
		helper::handle_error(
			mosquitto_publish(
				mosquitto_.get(), 
				nullptr,
				topic.c_str(),
				bytes.size(),
				bytes.data(),
				0,
				retain_message == retain::yes
			)
		);
	}
	
	private:
		mosquitto_ptr mosquitto_;
};

struct subscription
{
	template < typename T >
	explicit subscription( const host &h, const std::string &topic, T callback ) :
		mosquitto_( helper::handle_error( mosquitto_new( nullptr, true, this ) ) ),
		sc_( std::move( callback ) )
	{
		helper::handle_error(
			mosquitto_connect( 
				mosquitto_.get(), 
				h.host.c_str(), 
				h.port, 
				static_cast< int >( h.keep_alive_interval.count() )
			)
		);
		helper::handle_error(
			mosquitto_subscribe(
				mosquitto_.get(),
				nullptr,
				topic.c_str(),
				0
			)
		);
		mosquitto_message_callback_set(
			mosquitto_.get(),
			&mosquitto_callback
		);
	}
	
	void handle_events( std::chrono::milliseconds timeout = {} )
	{
		helper::handle_error(
			mosquitto_loop( mosquitto_.get(), timeout.count(), 1 )
		);
	}

	private:

		static void mosquitto_callback( struct mosquitto *, void *ptr, const mosquitto_message *m )
		{
			auto self = reinterpret_cast< subscription* >( ptr );
			self->sc_.call( m );
		}
		
		mosquitto_ptr mosquitto_;
		subscription_callback sc_;
};


}}