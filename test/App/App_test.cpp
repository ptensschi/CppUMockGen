/**
 * @file
 * @brief      Unit tests for the "App" class
 * @project    CppUMockGen
 * @authors    Jesus Gonzalez <jgonzalez@gdr-sistemas.com>
 * @copyright  Copyright (c) 2017 Jesus Gonzalez. All rights reserved.
 * @license    See LICENSE.txt
 */

/*===========================================================================
 *                              INCLUDES
 *===========================================================================*/

#include <CppUTest/TestHarness.h>
#include <CppUTestExt/MockSupport.h>

#include <string>
#include <vector>
#include <sstream>
#include <fstream>
#include <direct.h>

#include "App.hpp"
#include "FileHelper.hpp"

/*===========================================================================
 *                      COMMON TEST DEFINES & MACROS
 *===========================================================================*/

class StdVectorOfStringsComparator : public MockNamedValueComparator
{
public:
    bool isEqual(const void* object1, const void* object2)
    {
        const std::vector<std::string> *o1 = (const std::vector<std::string>*) object1;
        const std::vector<std::string> *o2 = (const std::vector<std::string>*) object2;

        return (*o1) == (*o2);
    }

    SimpleString valueToString(const void* object)
    {
        SimpleString ret;
        const std::vector<std::string> *o = (const std::vector<std::string>*) object;
        for( unsigned int i = 0; i < o->size(); i++ )
        {
            ret += StringFromFormat("<%u>%s\n", i, (*o)[i].c_str() );
        }
        return ret;
    }
};

StdVectorOfStringsComparator stdVectorOfStringsComparator;

class StdOstreamCopier : public MockNamedValueCopier
{
public:
    virtual void copy(void* out, const void* in)
    {
        *(std::ostream*)out << *(const std::string*)in;
    }
};

StdOstreamCopier stdOstreamCopier;

static const std::string tempDirPath = std::string(std::getenv("TEMP"));
static const std::string outDirPath = tempDirPath + PATH_SEPARATOR;
static const std::string inputFilename = "foo.h";
static const std::string mockOutputFilename = "foo_mock.cpp";
static const std::string mockOutputFilePath = outDirPath + mockOutputFilename;

/*===========================================================================
 *                          TEST GROUP DEFINITION
 *===========================================================================*/

TEST_GROUP( App )
{
    std::string initialDir;
    std::string outputFilepath;

    TEST_SETUP()
    {
        initialDir = getcwd( NULL, 0 );
    }

    TEST_TEARDOWN()
    {
        chdir( initialDir.c_str() );
        if( !outputFilepath.empty() )
        {
            std::remove( outputFilepath.c_str() );
        }
    }

    bool CheckFileContains( const std::string &filepath, const std::string &contents )
    {
        std::ifstream file( filepath );
        if( !file.is_open() )
        {
            return false;
        }
        std::stringstream strStream;
        strStream << file.rdbuf();

        return strStream.str() == contents;
    }
};

/*===========================================================================
 *                    TEST CASES IMPLEMENTATION
 *===========================================================================*/

/*
 * Check that help option displays usage
 */
TEST( App, Help )
{
    // Prepare
    std::ostringstream output;
    std::ostringstream error;
    App app( output, error );

    std::vector<const char *> args = { "CppUMockGen.exe", "-h" };

    // Exercise
    int ret = app.Execute( args.size(), args.data() );

    // Verify
    CHECK_EQUAL( 0, ret );
    STRCMP_CONTAINS( "Usage:", error.str().c_str() );
    CHECK_EQUAL( 0, output.tellp() );

    // Cleanup
}

/*
 * Check that if no input is specified, an error is displayed
 */
TEST( App, NoInput )
{
    // Prepare
    std::ostringstream output;
    std::ostringstream error;
    App app( output, error );

    std::vector<const char *> args = { "CppUMockGen.exe", "-x" };

    mock().expectNCalls(2, "ConsoleColorizer::SetColor").ignoreOtherParameters();

    // Exercise
    int ret = app.Execute( args.size(), args.data() );

    // Verify
    CHECK_EQUAL( 1, ret );
    STRCMP_CONTAINS( "ERROR:", error.str().c_str() );
    STRCMP_CONTAINS( "No input file specified", error.str().c_str() );
    CHECK_EQUAL( 0, output.tellp() );

    // Cleanup
}

/*
 * Check that if no output is specified, an error is displayed
 */
TEST( App, NoOutput )
{
    // Prepare
    std::ostringstream output;
    std::ostringstream error;
    App app( output, error );

    std::vector<const char *> args = { "CppUMockGen.exe", "-i", inputFilename.c_str() };

     mock().expectNCalls(2, "ConsoleColorizer::SetColor").ignoreOtherParameters();

    // Exercise
    int ret = app.Execute( args.size(), args.data() );

    // Verify
    CHECK_EQUAL( 1, ret );
    STRCMP_CONTAINS( "ERROR:", error.str().c_str() );
    STRCMP_CONTAINS( "At least the mock generation option (-m) or the expectation generation option (-e) must be specified",
                     error.str().c_str() );
    CHECK_EQUAL( 0, output.tellp() );

    // Cleanup
}

/*
 * Check that mock generation is requested properly and saved to an output directory (output filename deduced from input filename)
 */
TEST( App, MockOutput_OutDir )
{
    // Prepare
    mock().installComparator( "std::vector<std::string>", stdVectorOfStringsComparator );
    mock().installCopier( "std::ostream", stdOstreamCopier );

    outputFilepath = mockOutputFilePath;
    std::remove( outputFilepath.c_str() );

    std::ostringstream output;
    std::ostringstream error;
    App app( output, error );

    std::vector<const char *> args = { "CppUMockGen.exe", "-i", inputFilename.c_str(), "-m", outDirPath.c_str() };

    std::vector<std::string> paramOverrideOptions;
    std::vector<std::string> typeOverrideOptions;
    std::vector<std::string> includePaths;
	std::vector<std::string> defines;
    std::string outputText = "#####TEXT1#####";

    mock().expectOneCall("Config::Config").withBoolParameter("useUnderlyingTypedefType", false)
            .withParameterOfType("std::vector<std::string>", "paramOverrideOptions", &paramOverrideOptions)
            .withParameterOfType("std::vector<std::string>", "typeOverrideOptions", &typeOverrideOptions);
    mock().expectOneCall("Parser::Parse").withParameter("inputFilepath", inputFilename.c_str()).withParameter("interpretAsCpp", false)
            .withParameterOfType("std::vector<std::string>", "includePaths", &includePaths)
			.withParameterOfType("std::vector<std::string>", "defines", &defines).withPointerParameter("error", &error)
            .ignoreOtherParameters().andReturnValue(true);
    mock().expectOneCall("Parser::GenerateMock").withStringParameter("genOpts", "")
            .withOutputParameterOfTypeReturning("std::ostream", "output", &outputText);
    mock().expectNCalls(2, "ConsoleColorizer::SetColor").ignoreOtherParameters();

    // Exercise
    int ret = app.Execute( args.size(), args.data() );

    // Verify
    CHECK_EQUAL( 0, ret );
    STRCMP_CONTAINS( "SUCCESS:", error.str().c_str() );
    STRCMP_CONTAINS( ("Mock generated into '" + outputFilepath + "'").c_str(), error.str().c_str() );
    CHECK_EQUAL( 0, output.tellp() );
    CHECK( CheckFileContains( outputFilepath, outputText ) );

    // Cleanup
}

/*
 * Check that mock generation is requested properly and saved to the current directory (output filename deduced from input filename)
 */
TEST( App, MockOutput_CurrentDir )
{
    // Prepare
    mock().installComparator( "std::vector<std::string>", stdVectorOfStringsComparator );
    mock().installCopier( "std::ostream", stdOstreamCopier );

    const char* inputFilename = "bar";
    const std::string outputFilename = "bar_mock.cpp";
    outputFilepath = outDirPath + outputFilename;
    std::remove( outputFilepath.c_str() );

    std::ostringstream output;
    std::ostringstream error;
    App app( output, error );

    std::vector<const char *> args = { "CppUMockGen.exe", "-i", inputFilename, "-m" };

    std::vector<std::string> paramOverrideOptions;
    std::vector<std::string> typeOverrideOptions;
    std::vector<std::string> includePaths;
    std::vector<std::string> defines;
    std::string outputText = "#####TEXT2#####";

    chdir( tempDirPath.c_str() );

    mock().expectOneCall("Config::Config").withBoolParameter("useUnderlyingTypedefType", false)
            .withParameterOfType("std::vector<std::string>", "paramOverrideOptions", &paramOverrideOptions)
            .withParameterOfType("std::vector<std::string>", "typeOverrideOptions", &typeOverrideOptions);
    mock().expectOneCall("Parser::Parse").withParameter("inputFilepath", inputFilename).withParameter("interpretAsCpp", false)
            .withParameterOfType("std::vector<std::string>", "includePaths", &includePaths)
			.withParameterOfType("std::vector<std::string>", "defines", &defines).withPointerParameter("error", &error)
            .ignoreOtherParameters().andReturnValue(true);
    mock().expectOneCall("Parser::GenerateMock").withStringParameter("genOpts", "")
            .withOutputParameterOfTypeReturning("std::ostream", "output", &outputText);
    mock().expectNCalls(2, "ConsoleColorizer::SetColor").ignoreOtherParameters();

    // Exercise
    int ret = app.Execute( args.size(), args.data() );

    // Verify
    CHECK_EQUAL( 0, ret );
    STRCMP_CONTAINS( "SUCCESS:", error.str().c_str() );
    STRCMP_CONTAINS( ("Mock generated into '" + outputFilename + "'").c_str(), error.str().c_str() );
    CHECK_EQUAL( 0, output.tellp() );
    CHECK( CheckFileContains( outputFilepath, outputText ) );

    // Cleanup
}

/*
 * Check that mock generation is requested properly and saved to a named output file
 */
TEST( App, MockOutput_OutFile )
{
    // Prepare
    mock().installComparator( "std::vector<std::string>", stdVectorOfStringsComparator );
    mock().installCopier( "std::ostream", stdOstreamCopier );

    std::ostringstream output;
    std::ostringstream error;
    App app( output, error );

    outputFilepath = outDirPath + "mymock.cpp";
    std::remove( outputFilepath.c_str() );

    std::vector<const char *> args = { "CppUMockGen.exe", "-i", inputFilename.c_str(), "-m", outputFilepath.c_str() };

    std::vector<std::string> paramOverrideOptions;
    std::vector<std::string> typeOverrideOptions;
    std::vector<std::string> includePaths;
    std::vector<std::string> defines;
    std::string outputText = "#####TEXT3#####";

    mock().expectOneCall("Config::Config").withBoolParameter("useUnderlyingTypedefType", false)
            .withParameterOfType("std::vector<std::string>", "paramOverrideOptions", &paramOverrideOptions)
            .withParameterOfType("std::vector<std::string>", "typeOverrideOptions", &typeOverrideOptions);
    mock().expectOneCall("Parser::Parse").withParameter("inputFilepath", inputFilename.c_str()).withParameter("interpretAsCpp", false)
            .withParameterOfType("std::vector<std::string>", "includePaths", &includePaths)
			.withParameterOfType("std::vector<std::string>", "defines", &defines).withPointerParameter("error", &error)
            .ignoreOtherParameters().andReturnValue(true);
    mock().expectOneCall("Parser::GenerateMock").withStringParameter("genOpts", "")
            .withOutputParameterOfTypeReturning("std::ostream", "output", &outputText);
    mock().expectNCalls(2, "ConsoleColorizer::SetColor").ignoreOtherParameters();

    // Exercise
    int ret = app.Execute( args.size(), args.data() );

    // Verify
    CHECK_EQUAL( 0, ret );
    STRCMP_CONTAINS( "SUCCESS:", error.str().c_str() );
    STRCMP_CONTAINS( ("Mock generated into '" + outputFilepath + "'").c_str(), error.str().c_str() );
    CHECK_EQUAL( 0, output.tellp() );
    CHECK( CheckFileContains( outputFilepath, outputText ) );

    // Cleanup
}

/*
 * Check that mock generation is requested properly and printed to console
 */
TEST( App, MockOutput_ConsoleOutput )
{
    // Prepare
    mock().installComparator( "std::vector<std::string>", stdVectorOfStringsComparator );
    mock().installCopier( "std::ostream", stdOstreamCopier );

    std::ostringstream output;
    std::ostringstream error;
    App app( output, error );

    std::vector<const char *> args = { "CppUMockGen.exe", "-i", inputFilename.c_str(), "-m", "@" };

    std::vector<std::string> paramOverrideOptions;
    std::vector<std::string> typeOverrideOptions;
    std::vector<std::string> includePaths;
    std::vector<std::string> defines;
    std::string outputText = "#####TEXT4#####";

    mock().expectOneCall("Config::Config").withBoolParameter("useUnderlyingTypedefType", false)
            .withParameterOfType("std::vector<std::string>", "paramOverrideOptions", &paramOverrideOptions)
            .withParameterOfType("std::vector<std::string>", "typeOverrideOptions", &typeOverrideOptions);
    mock().expectOneCall("Parser::Parse").withParameter("inputFilepath", inputFilename.c_str()).withParameter("interpretAsCpp", false)
            .withParameterOfType("std::vector<std::string>", "includePaths", &includePaths)
			.withParameterOfType("std::vector<std::string>", "defines", &defines).withPointerParameter("error", &error)
            .ignoreOtherParameters().andReturnValue(true);
    mock().expectOneCall("Parser::GenerateMock").withStringParameter("genOpts", "")
            .withOutputParameterOfTypeReturning("std::ostream", "output", &outputText);

    // Exercise
    int ret = app.Execute( args.size(), args.data() );

    // Verify
    CHECK_EQUAL( 0, ret );
    STRCMP_EQUAL( outputText.c_str(), output.str().c_str() );
    CHECK_EQUAL( 0, error.tellp() );

    // Cleanup
}

/*
 * Check that if the output file cannot be opened, an error is displayed
 */
TEST( App, MockOutput_CannotOpenFile )
{
    // Prepare
    mock().installComparator( "std::vector<std::string>", stdVectorOfStringsComparator );
    mock().installCopier( "std::ostream", stdOstreamCopier );

    std::ostringstream output;
    std::ostringstream error;
    App app( output, error );

    std::string outputDir = outDirPath + "NonExistantDirectory123898876354874" + PATH_SEPARATOR;

    std::vector<const char *> args = { "CppUMockGen.exe", "-i", inputFilename.c_str(), "-m", outputDir.c_str() };

    mock().expectNCalls(2, "ConsoleColorizer::SetColor").ignoreOtherParameters();

   // Exercise
   int ret = app.Execute( args.size(), args.data() );

   // Verify
   CHECK_EQUAL( 1, ret );
   STRCMP_CONTAINS( "ERROR:", error.str().c_str() );
   STRCMP_CONTAINS( ("Mock output file '" + outputDir + mockOutputFilename + "' could not be opened").c_str(), error.str().c_str() );
   CHECK_EQUAL( 0, output.tellp() );

    // Cleanup
}

/*
 * Check that parsing in C++ mode is requested properly
 */
TEST( App, MockOutput_InterpretAsCpp )
{
    // Prepare
    mock().installComparator( "std::vector<std::string>", stdVectorOfStringsComparator );
    mock().installCopier( "std::ostream", stdOstreamCopier );

    std::ostringstream output;
    std::ostringstream error;
    App app( output, error );

    std::vector<const char *> args = { "CppUMockGen.exe", "-i", inputFilename.c_str(), "-m", "@", "-x" };

    std::vector<std::string> paramOverrideOptions;
    std::vector<std::string> typeOverrideOptions;
    std::vector<std::string> includePaths;
    std::vector<std::string> defines;
    std::string outputText = "#####FOO#####";

    mock().expectOneCall("Config::Config").withBoolParameter("useUnderlyingTypedefType", false)
            .withParameterOfType("std::vector<std::string>", "paramOverrideOptions", &paramOverrideOptions)
            .withParameterOfType("std::vector<std::string>", "typeOverrideOptions", &typeOverrideOptions);
    mock().expectOneCall("Parser::Parse").withParameter("inputFilepath", inputFilename.c_str()).withParameter("interpretAsCpp", true)
            .withParameterOfType("std::vector<std::string>", "includePaths", &includePaths)
			.withParameterOfType("std::vector<std::string>", "defines", &defines).withPointerParameter("error", &error)
            .ignoreOtherParameters().andReturnValue(true);
    mock().expectOneCall("Parser::GenerateMock").withStringParameter("genOpts", "-x ")
            .withOutputParameterOfTypeReturning("std::ostream", "output", &outputText);

    // Exercise
    int ret = app.Execute( args.size(), args.data() );

    // Verify
    CHECK_EQUAL( 0, ret );
    STRCMP_EQUAL( outputText.c_str(), output.str().c_str() );
    CHECK_EQUAL( 0, error.tellp() );

    // Cleanup
}

/*
 * Check that using underlying typedef types is requested properly
 */
TEST( App, MockOutput_UseUnderlyingTypedefType )
{
    // Prepare
    mock().installComparator( "std::vector<std::string>", stdVectorOfStringsComparator );
    mock().installCopier( "std::ostream", stdOstreamCopier );

    std::ostringstream output;
    std::ostringstream error;
    App app( output, error );

    std::vector<const char *> args = { "CppUMockGen.exe", "-i", inputFilename.c_str(), "-m", "@", "-u" };

    std::vector<std::string> paramOverrideOptions;
    std::vector<std::string> typeOverrideOptions;
    std::vector<std::string> includePaths;
    std::vector<std::string> defines;
    std::string outputText = "#####FOO#####";

    mock().expectOneCall("Config::Config").withBoolParameter("useUnderlyingTypedefType", true)
            .withParameterOfType("std::vector<std::string>", "paramOverrideOptions", &paramOverrideOptions)
            .withParameterOfType("std::vector<std::string>", "typeOverrideOptions", &typeOverrideOptions);
    mock().expectOneCall("Parser::Parse").withParameter("inputFilepath", inputFilename.c_str()).withParameter("interpretAsCpp", false)
            .withParameterOfType("std::vector<std::string>", "includePaths", &includePaths)
			.withParameterOfType("std::vector<std::string>", "defines", &defines).withPointerParameter("error", &error)
            .ignoreOtherParameters().andReturnValue(true);
    mock().expectOneCall("Parser::GenerateMock").withStringParameter("genOpts", "-u ")
            .withOutputParameterOfTypeReturning("std::ostream", "output", &outputText);

    // Exercise
    int ret = app.Execute( args.size(), args.data() );

    // Verify
    CHECK_EQUAL( 0, ret );
    STRCMP_EQUAL( outputText.c_str(), output.str().c_str() );
    CHECK_EQUAL( 0, error.tellp() );

    // Cleanup
}

/*
 * Check that include paths are passed properly to the parser
 */
TEST( App, MockOutput_IncludePaths )
{
    // Prepare
    mock().installComparator( "std::vector<std::string>", stdVectorOfStringsComparator );
    mock().installCopier( "std::ostream", stdOstreamCopier );

    std::ostringstream output;
    std::ostringstream error;
    App app( output, error );

    std::vector<const char *> args = { "CppUMockGen.exe", "-i", inputFilename.c_str(), "-m", "@", "-I", "IncludePath1", "-I", "IncludePath2" };

    std::vector<std::string> paramOverrideOptions;
    std::vector<std::string> typeOverrideOptions;
    std::vector<std::string> includePaths = { "IncludePath1", "IncludePath2" };
	std::vector<std::string> defines;
    std::string outputText = "#####FOO#####";

    mock().expectOneCall("Config::Config").withBoolParameter("useUnderlyingTypedefType", false)
            .withParameterOfType("std::vector<std::string>", "paramOverrideOptions", &paramOverrideOptions)
            .withParameterOfType("std::vector<std::string>", "typeOverrideOptions", &typeOverrideOptions);
    mock().expectOneCall("Parser::Parse").withParameter("inputFilepath", inputFilename.c_str()).withParameter("interpretAsCpp", false)
            .withParameterOfType("std::vector<std::string>", "includePaths", &includePaths)
			.withParameterOfType("std::vector<std::string>", "defines", &defines).withPointerParameter("error", &error)
            .ignoreOtherParameters().andReturnValue(true);
    mock().expectOneCall("Parser::GenerateMock").withStringParameter("genOpts", "")
            .withOutputParameterOfTypeReturning("std::ostream", "output", &outputText);

    // Exercise
    int ret = app.Execute( args.size(), args.data() );

    // Verify
    CHECK_EQUAL( 0, ret );
    STRCMP_EQUAL( outputText.c_str(), output.str().c_str() );
    CHECK_EQUAL( 0, error.tellp() );

    // Cleanup
}

/*
 * Check that preprocessor macro definitions are passed properly to the parser
 */
TEST( App, MockOutput_Defines )
{
    // Prepare
    mock().installComparator( "std::vector<std::string>", stdVectorOfStringsComparator );
    mock().installCopier( "std::ostream", stdOstreamCopier );

    std::ostringstream output;
    std::ostringstream error;
    App app( output, error );

    std::vector<const char *> args = { "CppUMockGen.exe", "-i", inputFilename.c_str(), "-m", "@", "-D", "DEFINE1", "-D", "DEFINE2" };

    std::vector<std::string> paramOverrideOptions;
    std::vector<std::string> typeOverrideOptions;
	std::vector<std::string> includePaths;
    std::vector<std::string> defines = { "DEFINE1", "DEFINE2" };
    std::string outputText = "#####FOO#####";

    mock().expectOneCall("Config::Config").withBoolParameter("useUnderlyingTypedefType", false)
            .withParameterOfType("std::vector<std::string>", "paramOverrideOptions", &paramOverrideOptions)
            .withParameterOfType("std::vector<std::string>", "typeOverrideOptions", &typeOverrideOptions);
    mock().expectOneCall("Parser::Parse").withParameter("inputFilepath", inputFilename.c_str()).withParameter("interpretAsCpp", false)
            .withParameterOfType("std::vector<std::string>", "includePaths", &includePaths)
			.withParameterOfType("std::vector<std::string>", "defines", &defines).withPointerParameter("error", &error)
            .ignoreOtherParameters().andReturnValue(true);
    mock().expectOneCall("Parser::GenerateMock").withStringParameter("genOpts", "")
            .withOutputParameterOfTypeReturning("std::ostream", "output", &outputText);

    // Exercise
    int ret = app.Execute( args.size(), args.data() );

    // Verify
    CHECK_EQUAL( 0, ret );
    STRCMP_EQUAL( outputText.c_str(), output.str().c_str() );
    CHECK_EQUAL( 0, error.tellp() );

    // Cleanup
}

/*
 * Check that include parameter override options are passed properly to the configuration
 */
TEST( App, MockOutput_paramOverrideOptions )
{
    // Prepare
    mock().installComparator( "std::vector<std::string>", stdVectorOfStringsComparator );
    mock().installCopier( "std::ostream", stdOstreamCopier );

    std::ostringstream output;
    std::ostringstream error;
    App app( output, error );

    std::vector<const char *> args = { "CppUMockGen.exe", "-i", inputFilename.c_str(), "-m", "@", "-p", "foo#bar=String", "-p", "foo@=Int/&$" };

    std::vector<std::string> paramOverrideOptions = { "foo#bar=String", "foo@=Int/&$" };
    std::vector<std::string> typeOverrideOptions;
    std::vector<std::string> includePaths;
    std::vector<std::string> defines;
    std::string outputText = "#####FOO#####";

    mock().expectOneCall("Config::Config").withBoolParameter("useUnderlyingTypedefType", false)
            .withParameterOfType("std::vector<std::string>", "paramOverrideOptions", &paramOverrideOptions)
            .withParameterOfType("std::vector<std::string>", "typeOverrideOptions", &typeOverrideOptions);
    mock().expectOneCall("Parser::Parse").withParameter("inputFilepath", inputFilename.c_str()).withParameter("interpretAsCpp", false)
            .withParameterOfType("std::vector<std::string>", "includePaths", &includePaths)
			.withParameterOfType("std::vector<std::string>", "defines", &defines).withPointerParameter("error", &error)
            .ignoreOtherParameters().andReturnValue(true);
    mock().expectOneCall("Parser::GenerateMock").withStringParameter("genOpts", "-p foo#bar=String -p foo@=Int/&$ ")
            .withOutputParameterOfTypeReturning("std::ostream", "output", &outputText);

    // Exercise
    int ret = app.Execute( args.size(), args.data() );

    // Verify
    CHECK_EQUAL( 0, ret );
    STRCMP_EQUAL( outputText.c_str(), output.str().c_str() );
    CHECK_EQUAL( 0, error.tellp() );

    // Cleanup
}

/*
 * Check that type parameter override options are passed properly to the configuration
 */
TEST( App, MockOutput_typeOverrideOptions )
{
    // Prepare
    mock().installComparator( "std::vector<std::string>", stdVectorOfStringsComparator );
    mock().installCopier( "std::ostream", stdOstreamCopier );

    std::ostringstream output;
    std::ostringstream error;
    App app( output, error );

    std::vector<const char *> args = { "CppUMockGen.exe", "-i", inputFilename.c_str(), "-m", "@", "-t", "#foo=String", "-t", "@const bar=Int/&$" };

    std::vector<std::string> paramOverrideOptions;
    std::vector<std::string> typeOverrideOptions = { "#foo=String", "@const bar=Int/&$" };
    std::vector<std::string> includePaths;
    std::vector<std::string> defines;
    std::string outputText = "#####FOO#####";

    mock().expectOneCall("Config::Config").withBoolParameter("useUnderlyingTypedefType", false)
            .withParameterOfType("std::vector<std::string>", "paramOverrideOptions", &paramOverrideOptions)
            .withParameterOfType("std::vector<std::string>", "typeOverrideOptions", &typeOverrideOptions);
    mock().expectOneCall("Parser::Parse").withParameter("inputFilepath", inputFilename.c_str()).withParameter("interpretAsCpp", false)
            .withParameterOfType("std::vector<std::string>", "includePaths", &includePaths)
			.withParameterOfType("std::vector<std::string>", "defines", &defines).withPointerParameter("error", &error)
            .ignoreOtherParameters().andReturnValue(true);
    mock().expectOneCall("Parser::GenerateMock").withStringParameter("genOpts", "-t #foo=String -t \"@const bar=Int/&$\" ")
            .withOutputParameterOfTypeReturning("std::ostream", "output", &outputText);

    // Exercise
    int ret = app.Execute( args.size(), args.data() );

    // Verify
    CHECK_EQUAL( 0, ret );
    STRCMP_EQUAL( outputText.c_str(), output.str().c_str() );
    CHECK_EQUAL( 0, error.tellp() );

    // Cleanup
}

/*
 * Check that mock generation is requested properly and printed to console
 */
TEST( App, MockOutput_ParseError )
{
    // Prepare
    mock().installComparator( "std::vector<std::string>", stdVectorOfStringsComparator );
    mock().installCopier( "std::ostream", stdOstreamCopier );

    std::ostringstream output;
    std::ostringstream error;
    App app( output, error );

    std::vector<const char *> args = { "CppUMockGen.exe", "-i", inputFilename.c_str(), "-m", "@" };

    std::vector<std::string> paramOverrideOptions;
    std::vector<std::string> typeOverrideOptions;
    std::vector<std::string> includePaths;
    std::vector<std::string> defines;
    std::string outputText = "#####TEXT4#####";

    mock().expectOneCall("Config::Config").withBoolParameter("useUnderlyingTypedefType", false)
            .withParameterOfType("std::vector<std::string>", "paramOverrideOptions", &paramOverrideOptions)
            .withParameterOfType("std::vector<std::string>", "typeOverrideOptions", &typeOverrideOptions);
    mock().expectOneCall("Parser::Parse").withParameter("inputFilepath", inputFilename.c_str()).withParameter("interpretAsCpp", false)
            .withParameterOfType("std::vector<std::string>", "includePaths", &includePaths)
			.withParameterOfType("std::vector<std::string>", "defines", &defines).withPointerParameter("error", &error)
            .ignoreOtherParameters().andReturnValue(false);
    mock().expectNCalls(2, "ConsoleColorizer::SetColor").ignoreOtherParameters();

    // Exercise
    int ret = app.Execute( args.size(), args.data() );

    // Verify
    CHECK_EQUAL( 2, ret );
    STRCMP_CONTAINS( "ERROR:", error.str().c_str() );
    STRCMP_CONTAINS( ("Output could not be generated due to errors parsing the input file '" + inputFilename + "'").c_str(), error.str().c_str() );
    CHECK_EQUAL( 0, output.tellp() );

    // Cleanup
}
