#include <map>


class ConfigFile final 
{
private:
	typedef std::map<std::string,std::string> ConfFileMap;
    ConfFileMap mCfgMap;
	std::string  mCfgFileName;

	void processLine( std::string& str );
	bool findString( const std::string& name, std::string& value ) const;
public:
	ConfigFile( const std::string& fileName );
	~ConfigFile();

	ConfigFile( const ConfigFile& ) = delete;
	ConfigFile( const ConfigFile&& ) = delete;

	static bool isFileExist( const std::string& fileName );
	//static bool isFileExist( const char* fileName );

	bool load( );

	void reset( );

	bool save();

	void setInt32ValueOf( const std::string& varName, const int32_t val );
	int32_t	int32ValueOf( const std::string& varName, int32_t defVal = 0 ) const;

	void setUint32ValueOf( const std::string& varName, const uint32_t val );
	uint32_t uint32ValueOf( const std::string& varName, uint32_t defVal = 0 ) const;

	void setInt64ValueOf( const std::string& varName, const int64_t val );
	int64_t	int64ValueOf( const std::string& varName, int64_t defVal = 0 ) const;

	void setUint64ValueOf( const std::string& varName, const uint64_t val );
	uint64_t uint64ValueOf( const std::string& varName, uint64_t defVal = 0 ) const;


	void setFloatValueOf( const std::string& varName, const float val );
	float floatValueOf( const std::string& varName, float defVal = 0.0f ) const;

	void setBoolValueOf( const std::string& varName, const bool val );
	bool boolValueOf( const std::string& varName, bool defVal = false ) const;

	void setStringValueOf( const std::string& varName, const std::string val );
	std::string  stringValueOf( const std::string& varName ) const;

	std::string cfgFileName( ) const;
};