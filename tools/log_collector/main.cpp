#include <windows.h>
#include <winreg.h>

#include <filesystem>
#include <string>
#include <iostream>
#include <fstream>
#include <sstream>
#include <vector>

#include "vdf_parser.hpp"

#define UNICODE
#define _UNICODE

// https://codereview.stackexchange.com/questions/419/converting-between-stdwstring-and-stdstring
std::string widestringtostring( const std::wstring& s )
{
    int len;
    int slength = (int)s.length() + 1;
    len = WideCharToMultiByte( CP_ACP, 0, s.c_str(), slength, 0, 0, 0, 0 );
    std::string r( len - 1, '\0' );
    WideCharToMultiByte( CP_ACP, 0, s.c_str(), slength, &r[0], len, 0, 0 );
    return r;
}

class RegistryError : public std::runtime_error
{
public:
    RegistryError( const char* message, LONG errorCode )
        : std::runtime_error( message )
        , _ErrorCode( errorCode )
    {
    }

    LONG ErrorCode() const noexcept
    {
        return _ErrorCode;
    }
private:
    LONG _ErrorCode;
};

// should probably open, then query key but idc
std::wstring RegGetString( HKEY hKey, const std::wstring& subkey, const std::wstring& value )
{
    DWORD retSize;
    LONG retCode = RegGetValueW(
        hKey,
        subkey.c_str(),
        value.c_str(),
        RRF_RT_REG_SZ,
        nullptr,
        nullptr,
        &retSize
    );

    if ( retCode != ERROR_SUCCESS )
        throw RegistryError( "Cannot read string from registry", retCode );

    std::wstring data;
    data.resize( retSize / sizeof( wchar_t ) );

    retCode = RegGetValueW(
        hKey,
        subkey.c_str(),
        value.c_str(),
        RRF_RT_REG_SZ,
        nullptr,
        &data[0],
        &retSize
    );

    if ( retCode != ERROR_SUCCESS )
        throw RegistryError( "Cannot read string from registry", retCode );

    data.resize( (DWORD)(retSize / sizeof( wchar_t )) - 1 );

    return data;
}


std::filesystem::path FindSteam()
{
    // Find steam's install location

    std::cout << "Attempting to find steam 32 bit..." << std::endl;
    std::cout << "Reading registry HKEY_LOCAL_MACHINE\\SOFTWARE\\Valve\\Steam" << std::endl;
    std::string steampath = "";
    try
    {
        steampath = widestringtostring( RegGetString( HKEY_LOCAL_MACHINE, L"SOFTWARE\\Valve\\Steam", L"InstallPath" ) );
    }
    catch ( RegistryError e )
    {
        std::cout << "32 bit steam not found" << std::endl;
        std::cout << "Attempting to find steam 64 bit..." << std::endl;
        std::cout << "Reading registry HKEY_LOCAL_MACHINE\\SOFTWARE\\WOW6432Node\\Valve\\Steam" << std::endl;
        try
        {
            steampath = widestringtostring( RegGetString( HKEY_LOCAL_MACHINE, L"HKEY_LOCAL_MACHINE\\SOFTWARE\\Wow6432Node\\Valve\\Steam", L"InstallPath" ) );
        }
        catch ( RegistryError e )
        {
            std::cout << "Steam not found" << std::endl;
            exit( 0 );
        }
    }

    return std::filesystem::path{ steampath };
}

std::filesystem::path FindGame()
{
    // Find steam's install location

    std::string steampath = FindSteam().string();
    std::cout << "Steam found at " << steampath << std::endl;

    // find possible game install locations from steam other than steamapps/common/
    
    std::stringstream libraryfoldersvdfpath;
    libraryfoldersvdfpath << steampath << "\\steamapps\\libraryfolders.vdf";
    std::cout << "Attempting to read " << libraryfoldersvdfpath.str() << "..." << std::endl;

    std::ifstream iflibraryfoldersvdf( libraryfoldersvdfpath.str() );

    // push back all the entries in libraryfolder.vdf
    std::cout << "Attempting to parse other steam libraries..." << std::endl;
    std::vector<std::string> steamlibrarylocations;

    // find basic steam library

    std::stringstream basicsteamlibrarypath;
    basicsteamlibrarypath << steampath << "\\steamapps\\common";
    std::cout << "Steam library found at " << basicsteamlibrarypath.str() << std::endl;
    steamlibrarylocations.push_back( basicsteamlibrarypath.str() );

    auto root = tyti::vdf::read( iflibraryfoldersvdf );

    for ( int i = 1; ; i++ )
    {
        auto got = root.attribs.find( std::to_string( i ) );
        if ( got == root.attribs.end() ) break;
        std::string location = root.attribs[std::to_string( i )];
        std::stringstream locationbuilder;
        locationbuilder << location << "\\steamapps\\common";
        std::cout << "Steam library found at " << locationbuilder.str() << std::endl;
        steamlibrarylocations.push_back( locationbuilder.str() );
    }

    std::cout << "No other steam libraries found" << std::endl;

    // which of these has a Fallout 4 folder in /steamapps/common

    std::filesystem::path falloutlocation;
    for ( auto& librarylocation : steamlibrarylocations )
    {
        std::cout << "Attempting to find Fallout 4 in " << librarylocation << std::endl;
        std::stringstream locationbuilder;
        locationbuilder << librarylocation << "\\Fallout\ 4";
        bool doesexist = std::filesystem::exists( std::filesystem::path{ locationbuilder.str() } );
        if ( doesexist )
        {
            std::cout << "Found Fallout 4! at " << locationbuilder.str() << std::endl;
            falloutlocation = std::filesystem::path{ locationbuilder.str() };
            break;
        }
    }

    return falloutlocation;
}

int main( int argc, char** argv )
{
    std::filesystem::path gamepath = FindGame();
    std::cout << std::endl;

    std::stringstream falloutlog;
    falloutlog << gamepath.string() << "\\Logs\\Script\\EditorPapyrus.0.log";

    std::cout << "Attempting to read " << falloutlog.str() << std::endl;
    if ( std::filesystem::exists( std::filesystem::path{ falloutlog.str() } ) )
    {
        // read log
        std::cout << "Reading " << falloutlog.str() << std::endl << std::endl;
        std::ifstream iffalloutlog( falloutlog.str() );
        std::string line;
        while ( getline( iffalloutlog, line ) )
        {
            std::cout << line << std::endl;
        }

        std::cout << std::endl;
    }
    else
    {
        std::cout << "Cannot find " << falloutlog.str() << std::endl;
    }

    std::vector<std::filesystem::path> F4SELogs;
    std::cout << "Attempting to read " << falloutlog.str() << std::endl;
    std::cout << "Attempting to read " << falloutlog.str() << std::endl;
    std::cout << "Attempting to read " << falloutlog.str() << std::endl;

    getchar();
}
