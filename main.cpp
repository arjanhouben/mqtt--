#include <iostream>
#include <system_error>
#include <arjan/mqttpp.hpp>

template < int Signal >
struct catch_signal
{
	explicit catch_signal()
	{
		static auto &ref = caught_;
		if ( signal( Signal, [](int){ ref = true; } ) == SIG_ERR )
		{
			throw std::system_error( errno, std::generic_category() );
		}
	}

	explicit operator bool() const
	{
		return caught_;
	}

	private:
		volatile bool caught_;
};

int main( int argc, char *argv[] )
{
	try
	{
		arjan::mqttpp::host host;
		arjan::mqttpp::subscription c( 
			{},
			"test/#",
			[]( const auto &m )
			{
				std::cout << m.topic << '\n';
			}
		);

		arjan::mqttpp::publisher p( host );

		p.publish( "test", std::span{ "test" } );

		catch_signal< SIGINT > signal;

		while ( !signal )
		{
			c.handle_events();
		}
	}
	catch( std::exception &err )
	{
		std::cerr << err.what() << std::endl;
		return 1;
	}
	return 0;
}