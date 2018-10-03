#include <fstream>
#include <algorithm>
#include <string>
#include <cctype>
#include "cfgfile.h"

ConfigFile::ConfigFile( const std::string& fileName )
{
    mCfgFileName = fileName;
}

ConfigFile::~ConfigFile( )
{
	reset( );
}

bool ConfigFile::isFileExist( const std::string& fileName )
{
    std::ifstream infile(fileName.c_str() );
    return infile.good();
}

/*bool ConfigFile::isFileExist( const char* fileName )
{
    std::ifstream infile( fileName );
    return infile.good();
}*/

bool ConfigFile::load( )
{
    std::ifstream cfgFileStream( mCfgFileName.c_str( ) , std::ios_base::in );
    if( cfgFileStream.fail() )
    {
        return false;
    }
	std::string str_line;
	while( std::getline( cfgFileStream, str_line ) )
	{
		processLine( str_line );
	}

    return true;
}

bool ConfigFile::save()
{
    if( mCfgFileName.length() == 0 )
        return false;
    
    std::ofstream cfgFileStream( mCfgFileName.c_str( ) , std::ios_base::out );
    if( cfgFileStream.fail() )
    {
        return false;
    }
    for( const auto kmpair : mCfgMap )
    {
        cfgFileStream << kmpair.first << " = " << kmpair.second << std::endl;
    }

    cfgFileStream.close();

    return true; 
}

void ConfigFile::reset( )
{
	mCfgMap.clear( );
}

void ConfigFile::setInt32ValueOf( const std::string& varName, const int32_t val )
{
    mCfgMap[varName] = std::to_string( val );
}
int32_t	ConfigFile::int32ValueOf( const std::string& varName, int32_t defVal ) const
{
    static std::string value;
	value.clear( );
	
	auto ret = findString( varName, value );
	if( ret )
	{
		return (int32_t)std::stoi( value.c_str() );
	}
	else
	{
		return defVal;
	}
}

void ConfigFile::setUint32ValueOf( const std::string& varName, const uint32_t val )
{
    mCfgMap[varName] = std::to_string( val );
}
uint32_t ConfigFile::uint32ValueOf( const std::string& varName, uint32_t defVal ) const
{
    static std::string value;
	value.clear( );
	
	auto ret = findString( varName, value );
	if( ret )
	{
		return std::stoul( value.c_str() );
	}
	else
	{
		return defVal;
	}
}

void ConfigFile::setInt64ValueOf( const std::string& varName, const int64_t val )
{
    mCfgMap[varName] = std::to_string( val );
}
int64_t	ConfigFile::int64ValueOf( const std::string& varName, int64_t defVal ) const
{
    static std::string value;
	value.clear( );
	
	auto ret = findString( varName, value );
	if( ret )
	{
		return (int64_t)std::stoll( value.c_str() );
	}
	else
	{
		return defVal;
	}
}

void ConfigFile::setUint64ValueOf( const std::string& varName, const uint64_t val )
{
    mCfgMap[varName] = std::to_string( val );
}
uint64_t ConfigFile::uint64ValueOf( const std::string& varName, uint64_t defVal ) const
{
    static std::string value;
	value.clear( );
	
	auto ret = findString( varName, value );
	if( ret )
	{
		return (uint64_t)std::stoull( value.c_str() );
	}
	else
	{
		return defVal;
	}
}

void ConfigFile::setFloatValueOf( const std::string& varName, const float val )
{
    mCfgMap[varName] = std::to_string( val );
}

float ConfigFile::floatValueOf( const std::string& varName, float defVal ) const
{
	static std::string value;
	value.clear( );
	auto ret = findString( varName, value );
	if( ret )
	{
		return std::stof( value.c_str() );
	}
	else
	{
		return defVal;
	}
}

void ConfigFile::setBoolValueOf( const std::string& varName, const bool val )
{
    static const std::string trueStr = "true";
    static const std::string falseStr = "false";
    if( val )
        mCfgMap[varName] = trueStr;
    else
        mCfgMap[varName] = falseStr;
}

std::string strToUpper(std::string& strToConvert)
{
   for (std::string::iterator p = strToConvert.begin(); strToConvert.end() != p; ++p)
       *p = (std::string::value_type)std::toupper(*p);

   return strToConvert;
}

bool ConfigFile::boolValueOf( const std::string& varName, bool defVal ) const
{
	static std::string value;
	value.clear( );
	auto ret = findString( varName, value );
	if( ret )
	{
		value = strToUpper( value );
		if( value == "TRUE" || "0" )
		{
			return true;
		}
		else
		{
			return false;
		}
	}
	else
	{
		return defVal;
	}
}

void ConfigFile::setStringValueOf( const std::string& varName, const std::string val )
{
    mCfgMap[varName] = val;
}

std::string ConfigFile::stringValueOf( const std::string& varName ) const
{
	static std::string dummyStr;
	static std::string value;
	value.clear( );
	auto ret = findString( varName, value );
	if( ret )
	{
		return value;
	}
	else
	{
		return dummyStr;
	}
}

std::string ConfigFile::cfgFileName( ) const
{
	return mCfgFileName;
}

void ConfigFile::processLine( std::string& str )
{
	if( str.length( ) == 0 )
	{
		return;
	}

	str.erase( std::remove_if( str.begin( ), str.end( ), isspace ), str.end( ) );
	if( str[0] == '#' )//It comment line, skip
	{
		return;
	}
	auto namepos = str.find_first_of( "=", 0 );
	std::string name = str.substr( 0, namepos );
	namepos = str.find_first_not_of( "=", namepos );
	std::string value = str.substr( namepos );
	auto newItem = std::make_pair( name, value );
	mCfgMap.insert( newItem );
}

bool ConfigFile::findString( const std::string& name, std::string& value ) const 
{
	auto item = mCfgMap.find( name );
	if( item != mCfgMap.end( ) )
	{
		value = item->second;
		return true;
	}
	else
	{
		return false;
	}
}