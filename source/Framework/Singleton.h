#pragma once

template <typename T> class Singleton
{
public:							
	static T& Instance()
	{
		static T ms_instance;
		return ms_instance;
	}

	protected:
//		Singleton( void )
//		{
//		}
};
