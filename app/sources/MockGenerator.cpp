#include "MockGenerator.hpp"

#include <iostream>
#include <clang-c/Index.h>

#include "Function.hpp"
#include "Method.hpp"
#include "ClangHelper.hpp"
#include "ConsoleColorizer.hpp"

struct ParseData
{
    const Config& config;
    std::ostream &output;
};

#ifdef WIN32
#define PATH_SEPARATOR '\\'
#else
#define PATH_SEPARATOR '/'
#endif

static std::string GetFilenameFromPath( const std::string& filepath )
{
    size_t sepPos = filepath.rfind( PATH_SEPARATOR );
    if( sepPos == std::string::npos )
    {
        return filepath;
    }
    else
    {
        return filepath.substr( sepPos + 1 );
    }
}

bool GenerateMock( const std::string &inputFilename, const Config &config, bool interpretAsCpp,
                   const std::vector<std::string> &includePaths, std::ostream &output )
{
    CXIndex index = clang_createIndex( 0, 0 );

    std::vector<const char*> clangOpts;
    if( interpretAsCpp )
    {
        clangOpts.push_back( "-xc++" );
    }

    std::vector<std::string> includePathOpts;

    for( const std::string &includePath : includePaths )
    {
        includePathOpts.push_back( "-I" + includePath );
        clangOpts.push_back( includePathOpts.back().c_str() );
    }

    CXTranslationUnit tu = clang_parseTranslationUnit( index, inputFilename.c_str(),
                                                       clangOpts.data(), clangOpts.size(),
                                                       nullptr, 0,
                                                       CXTranslationUnit_SkipFunctionBodies );
    if( tu == nullptr )
    {
        std::cerr << "Unable to parse translation unit. Quitting." << std::endl;
        exit(-1);
    }

    unsigned int numDiags = clang_getNumDiagnostics(tu);
    unsigned int numErrors = 0;
    if( numDiags > 0 )
    {
        for( unsigned int i = 0; i < numDiags; i++ )
        {
            CXDiagnostic diag = clang_getDiagnostic( tu, i );

            CXDiagnosticSeverity diagSeverity = clang_getDiagnosticSeverity( diag );

            switch( diagSeverity )
            {
                case CXDiagnosticSeverity::CXDiagnostic_Fatal:
                case CXDiagnosticSeverity::CXDiagnostic_Error:
                    numErrors++;
                    cerrColorizer.SetColor( ConsoleColorizer::Color::LIGHT_RED );
                    std::cerr << "PARSE ERROR: ";
                    break;

                case CXDiagnosticSeverity::CXDiagnostic_Warning:
                    cerrColorizer.SetColor( ConsoleColorizer::Color::YELLOW );
                    std::cerr << "PARSE WARNING: ";
                    break;

                default:
                    break;
            }

            cerrColorizer.SetColor( ConsoleColorizer::Color::RESET );

            std::cerr << clang_formatDiagnostic( diag, clang_defaultDiagnosticDisplayOptions() ) << std::endl;

            clang_disposeDiagnostic( diag );
        }
    }

    if( numErrors == 0 )
    {
        output << "/* This file has been auto-generated by CppUTestMock. DO NOT EDIT IT!!! */" << std::endl;
        output << std::endl;
        if( !interpretAsCpp )
        {
            output << "extern \"C\" {" << std::endl;
        }
        output << "#include \"" <<  GetFilenameFromPath( inputFilename ) << "\"" << std::endl;
        if( !interpretAsCpp )
        {
            output << "}" << std::endl;
        }
        output << std::endl;
        output << "#include <CppUTestExt/MockSupport.h>" << std::endl;
        output << std::endl;

        ParseData parseData = { config, output };

        CXCursor tuCursor = clang_getTranslationUnitCursor(tu);
        clang_visitChildren(
            tuCursor,
            []( CXCursor cursor, CXCursor parent, CXClientData clientData )
            {
                ParseData *parseData = (ParseData*) clientData;
                if( clang_Location_isFromMainFile( clang_getCursorLocation( cursor ) ) != 0 )
                {
                    CXCursorKind cursorKind = clang_getCursorKind( cursor );
                    if( cursorKind == CXCursor_FunctionDecl )
                    {
                        Function function( cursor, parseData->config );
                        if( function.IsMockable() )
                        {
                            parseData->output << function.GenerateMock() << std::endl;
                        }
                        return CXChildVisit_Continue;
                    }
                    else if( cursorKind == CXCursor_CXXMethod )
                    {
                        Method method( cursor, parseData->config );
                        if( method.IsMockable() )
                        {
                            parseData->output << method.GenerateMock() << std::endl;
                        }
                        return CXChildVisit_Continue;
                    }
                    else
                    {
                        //            std::cout << "Cursor '" << clang_getCursorSpelling( cursor ) << "' of kind '"
                        //                << clang_getCursorKindSpelling( clang_getCursorKind( cursor ) ) << "'\n";
                        return CXChildVisit_Recurse;
                    }
                }
                else
                {
                    return CXChildVisit_Continue;
                }
            },
            (CXClientData) &parseData );
    }

    clang_disposeTranslationUnit( tu );
    clang_disposeIndex( index );

    return (numErrors == 0);
}