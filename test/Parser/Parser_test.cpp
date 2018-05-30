/**
 * @file
 * @brief      unit tests for the "Method" class
 * @project    CppUMockGen
 * @authors    Jesus Gonzalez <jgonzalez@gdr-sistemas.com>
 * @copyright  Copyright (c) 2017 Jesus Gonzalez. All rights reserved.
 * @license    See LICENSE.txt
 */

/*===========================================================================
 *                              INCLUDES
 *===========================================================================*/

#include <map>

#include <fstream>
#include <sstream>
#include <iostream>
#include <string>
#include <vector>
#include <cstdlib>
#include <direct.h>

#include "ClangParseHelper.hpp"
#include "ClangCompileHelper.hpp"

#include <CppUTest/TestHarness.h>
#include <CppUTestExt/MockSupport.h>

#include "Parser.hpp"
#include "FileHelper.hpp"

#ifdef _MSC_VER
#pragma warning( disable : 4996 )
#endif

/*===========================================================================
 *                      COMMON TEST DEFINES & MACROS
 *===========================================================================*/

Config* GetMockConfig()
{
    return (Config*) (void*) 836487567;
}

/*===========================================================================
 *                          TEST GROUP DEFINITION
 *===========================================================================*/

static const std::string tempDirPath = std::string(std::getenv("TEMP"));
static const std::string tempFilename = "CppUMockGen_MockGenerator.h";
static const std::string tempFilePath = tempDirPath + PATH_SEPARATOR + tempFilename;
static const std::string nonexistingFilePath = tempDirPath + PATH_SEPARATOR + "CppUMockGen_MockGenerator_NotExisting.h";

TEST_GROUP( MockGenerator )
{
    std::ofstream tempFile;
    std::string initialDir;

    TEST_SETUP()
    {
        initialDir = getcwd( NULL, 0 );
    }

    TEST_TEARDOWN()
    {
        chdir( initialDir.c_str() );
        std::remove( tempFilePath.c_str() );
    }

    void SetupTempFile( const SimpleString& contents )
    {
        tempFile.open( tempFilePath );
        tempFile << contents.asCharString();
        tempFile.close();
    }
};

/*===========================================================================
 *                    TEST CASES IMPLEMENTATION
 *===========================================================================*/

/*
 * Check that mocking a function works as expected.
 */
TEST( MockGenerator, MockedFunction )
{
   // Prepare
   Config* config = GetMockConfig();
   std::ostringstream error;

   SimpleString testHeader =
           "void function1(int a);";
   SetupTempFile( testHeader );

   mock().expectOneCall("Function::Parse").withConstPointerParameter("config", config).ignoreOtherParameters().andReturnValue(true);

   // Exercise
   Parser parser;
   bool result = parser.Parse( tempFilePath, *config, false, false, std::vector<std::string>(), std::vector<std::string>(), error );

   // Verify
   CHECK_EQUAL( true, result );
   CHECK_EQUAL( 0, error.tellp() )
   mock().checkExpectations();

   // Prepare
   std::ostringstream output;
   const char* testMock = "###MOCK###";

   mock().expectOneCall("Function::GenerateMock").andReturnValue(testMock);

   // Exercise
   parser.GenerateMock( "", output );

   // Verify
   STRCMP_CONTAINS( testMock, output.str().c_str() );
   STRCMP_CONTAINS( "extern \"C\"", output.str().c_str() );

   // Cleanup
}

/*
 * Check that mocking a method works as expected.
 */
TEST( MockGenerator, MockedMethod )
{
    // Prepare
    Config* config = GetMockConfig();
    std::ostringstream error;

    SimpleString testHeader =
            "class class1 {\n"
            "public:\n"
            "    void method1();\n"
            "};";
    SetupTempFile( testHeader );

    mock().expectOneCall("Function::Parse").withConstPointerParameter("config", config).ignoreOtherParameters().andReturnValue(true);

    // Exercise
    Parser parser;
    bool result = parser.Parse( tempFilePath, *config, true, false, std::vector<std::string>(), std::vector<std::string>(), error );

    // Verify
    CHECK_EQUAL( true, result );
    CHECK_EQUAL( 0, error.tellp() )
    mock().checkExpectations();

    // Prepare
    std::ostringstream output;
    const char* testMock = "###MOCK###";

    mock().expectOneCall("Function::GenerateMock").andReturnValue(testMock);

    // Exercise
    parser.GenerateMock( "", output );

    // Verify
    STRCMP_CONTAINS( testMock, output.str().c_str() );
    // Cleanup
}

/*
 * Check that mocking a method works as expected.
 */
TEST( MockGenerator, MultipleMockableFunctionsAndMethods )
{
    // Prepare
    Config* config = GetMockConfig();
    std::ostringstream error;

    SimpleString testHeader =
            "void function1(int a);\n"
            "int function2();\n"
            "class class1 {\n"
            "public:\n"
            "    void method1();\n"
            "    double method2(int*);\n"
            "};";
    SetupTempFile( testHeader );

    mock().expectNCalls(2, "Function::Parse").withConstPointerParameter("config", config).ignoreOtherParameters().andReturnValue(true);
    mock().expectNCalls(2, "Function::Parse").withConstPointerParameter("config", config).ignoreOtherParameters().andReturnValue(true);

    // Exercise
    Parser parser;
    bool result = parser.Parse( tempFilePath, *config, true, false, std::vector<std::string>(), std::vector<std::string>(), error );

    // Verify
    CHECK_EQUAL( true, result );
    CHECK_EQUAL( 0, error.tellp() )
    mock().checkExpectations();

    // Prepare
    std::ostringstream output;
    const char* testMock[] = { "### MOCK 1 ###\n", "### MOCK 2 ###\n", "### MOCK 3 ###\n", "### MOCK 4 ###\n" };

    mock().expectOneCall("Function::GenerateMock").andReturnValue(testMock[0]);
    mock().expectOneCall("Function::GenerateMock").andReturnValue(testMock[1]);
    mock().expectOneCall("Function::GenerateMock").andReturnValue(testMock[2]);
    mock().expectOneCall("Function::GenerateMock").andReturnValue(testMock[3]);

    // Exercise
    parser.GenerateMock( "", output );

    // Verify
    STRCMP_CONTAINS( testMock[0], output.str().c_str() );
    STRCMP_CONTAINS( testMock[1], output.str().c_str() );
    STRCMP_CONTAINS( testMock[2], output.str().c_str() );
    STRCMP_CONTAINS( testMock[3], output.str().c_str() );

    // Cleanup
}

/*
 * Check that mocking a non-mockable function works as expected.
 */
TEST( MockGenerator, FunctionNonMockable )
{
   // Prepare
   Config* config = GetMockConfig();
   std::ostringstream error;

   SimpleString testHeader =
           "void function1(int a);";
   SetupTempFile( testHeader );

   mock().expectOneCall("Function::Parse").withConstPointerParameter("config", config).ignoreOtherParameters().andReturnValue(false);
   mock().expectNCalls(2, "ConsoleColorizer::SetColor").ignoreOtherParameters();

   // Exercise
   Parser parser;
   bool result = parser.Parse( tempFilePath, *config, false, false, std::vector<std::string>(), std::vector<std::string>(), error );

   // Verify
   CHECK_EQUAL( false, result );
   STRCMP_CONTAINS( "INPUT ERROR:", error.str().c_str() );
   STRCMP_CONTAINS( "The input file does not contain any mockable function", error.str().c_str() );

   // Cleanup
}

/*
 * Check that mocking a non-mockable method works as expected.
 */
TEST( MockGenerator, MethodNonMockable )
{
   // Prepare
   Config* config = GetMockConfig();
   std::ostringstream error;

   SimpleString testHeader =
           "class class1 {\n"
           "public:\n"
           "    void method1();\n"
           "};";
   SetupTempFile( testHeader );

   mock().expectOneCall("Function::Parse").withConstPointerParameter("config", config).ignoreOtherParameters().andReturnValue(false);
   mock().expectNCalls(2, "ConsoleColorizer::SetColor").ignoreOtherParameters();

   // Exercise
   Parser parser;
   bool result = parser.Parse( tempFilePath, *config, true, false, std::vector<std::string>(), std::vector<std::string>(), error );

   // Verify
   CHECK_EQUAL( false, result );
   STRCMP_CONTAINS( "INPUT ERROR:", error.str().c_str() );
   STRCMP_CONTAINS( "The input file does not contain any mockable function", error.str().c_str() );

   // Cleanup
}

/*
 * Check that mocking a method works as expected.
 */
TEST( MockGenerator, MixedMockableNonMockableFunctionsAndMethods )
{
    // Prepare
    Config* config = GetMockConfig();
    std::ostringstream error;

    SimpleString testHeader =
            "void function1(int a);\n"
            "int function2();\n"
            "class class1 {\n"
            "public:\n"
            "    void method1();\n"
            "    double method2(int*);\n"
            "};";
    SetupTempFile( testHeader );

    mock().expectOneCall("Function::Parse").withConstPointerParameter("config", config).ignoreOtherParameters().andReturnValue(true);
    mock().expectOneCall("Function::Parse").withConstPointerParameter("config", config).ignoreOtherParameters().andReturnValue(false);
    mock().expectOneCall("Function::Parse").withConstPointerParameter("config", config).ignoreOtherParameters().andReturnValue(true);
    mock().expectOneCall("Function::Parse").withConstPointerParameter("config", config).ignoreOtherParameters().andReturnValue(false);

    // Exercise
    Parser parser;
    bool result = parser.Parse( tempFilePath, *config, true, false, std::vector<std::string>(), std::vector<std::string>(), error );

    // Verify
    CHECK_EQUAL( true, result );
    CHECK_EQUAL( 0, error.tellp() )
    mock().checkExpectations();

    // Prepare
    std::ostringstream output;
    const char* testMock[] = { "### MOCK 1 ###\n", "### MOCK 2 ###\n" };

    mock().expectOneCall("Function::GenerateMock").andReturnValue(testMock[0]);
    mock().expectOneCall("Function::GenerateMock").andReturnValue(testMock[1]);

    // Exercise
    parser.GenerateMock( "", output );

    // Verify
    STRCMP_CONTAINS( testMock[0], output.str().c_str() );
    STRCMP_CONTAINS( testMock[1], output.str().c_str() );

    // Cleanup
}

/*
 * Check that a syntax error aborts mock generation.
 */
TEST( MockGenerator, SyntaxError )
{
   // Prepare
   Config* config = GetMockConfig();
   std::ostringstream error;

   SimpleString testHeader =
           "foo function1(int a);";
   SetupTempFile( testHeader );

   mock().expectNCalls(2, "ConsoleColorizer::SetColor").ignoreOtherParameters();

   // Exercise
   Parser parser;
   bool result = parser.Parse( tempFilePath, *config, false, false, std::vector<std::string>(), std::vector<std::string>(), error );

   // Verify
   CHECK_EQUAL( false, result );
   STRCMP_CONTAINS( "PARSE ERROR:", error.str().c_str() );
   STRCMP_CONTAINS( "CppUMockGen_MockGenerator.h:1:1: error: unknown type name 'foo'", error.str().c_str() );

   // Cleanup
}

/*
 * Check that a warning is handled without aborting mock generation.
 */
TEST( MockGenerator, Warning )
{
   // Prepare
   Config* config = GetMockConfig();
   std::ostringstream output;
   std::ostringstream error;
   const char* testMock = "###MOCK###";

   SimpleString testHeader =
           "#warning test\n"
           "void function1(int a);";
   SetupTempFile( testHeader );

   mock().expectNCalls(2, "ConsoleColorizer::SetColor").ignoreOtherParameters();
   mock().expectOneCall("Function::Parse").withConstPointerParameter("config", config).ignoreOtherParameters().andReturnValue(true);
   mock().expectOneCall("Function::GenerateMock").andReturnValue(testMock);

   // Exercise
   std::vector<std::string> results;
   Parser parser;
   bool result = parser.Parse( tempFilePath, *config, false, false, std::vector<std::string>(), std::vector<std::string>(), error );
   parser.GenerateMock( "", output );

   // Verify
   CHECK_EQUAL( true, result );
   STRCMP_CONTAINS( testMock, output.str().c_str() );
   STRCMP_CONTAINS( "PARSE WARNING:", error.str().c_str() );
   STRCMP_CONTAINS( "CppUMockGen_MockGenerator.h:1:2: warning: test [-W#warnings]", error.str().c_str() );
   mock().checkExpectations();

   // Cleanup
}

/*
 * Check that an error is issued when the input file does not exist.
 */
TEST( MockGenerator, NonExistingInputFile )
{
   // Prepare
   Config* config = GetMockConfig();
   std::ostringstream error;

   std::remove( nonexistingFilePath.c_str() );

   mock().expectNCalls(2, "ConsoleColorizer::SetColor").ignoreOtherParameters();

   // Exercise
   Parser parser;
   bool result = parser.Parse( nonexistingFilePath, *config, false, false, std::vector<std::string>(), std::vector<std::string>(), error );

   // Verify
   CHECK_EQUAL( false, result );
   STRCMP_CONTAINS( "INPUT ERROR: Input file '", error.str().c_str() );
   STRCMP_CONTAINS( "CppUMockGen_MockGenerator_NotExisting.h' does not exist", error.str().c_str() );

   // Cleanup
}

/*
 * Check that include paths are processed properly.
 */
TEST( MockGenerator, IncludePaths )
{
   // Prepare
   Config* config = GetMockConfig();
   std::ostringstream error;

   std::string includePath = std::string(PROD_DIR) + PATH_SEPARATOR + "sources";

   SimpleString testHeader =
           "#include \"Config.hpp\"\n"
           "void method1(Config &c);\n";
   SetupTempFile( testHeader );

   chdir( tempDirPath.c_str() );

   mock().expectOneCall("Function::Parse").withConstPointerParameter("config", config).ignoreOtherParameters().andReturnValue(true);

   // Exercise
   Parser parser;
   bool result = parser.Parse( tempFilename, *config, true, false, std::vector<std::string>{includePath}, std::vector<std::string>(), error );

   // Verify
   CHECK_EQUAL( true, result );
   CHECK_EQUAL( 0, error.tellp() )

   // Cleanup
}

/*
 * Check that preprocessor macro definitions are processed properly.
 */
IGNORE_TEST( MockGenerator, PreprocessorMacroDefinitions )
{
   // Prepare
   Config* config = GetMockConfig();
   std::ostringstream error;

   std::string define = "SOME_DEFINE";

   SimpleString testHeader =
           "#ifndef SOME_DEFINE\n"
           "#error Some error;\n"
           "#endif\n";
   SetupTempFile( testHeader );

   chdir( tempDirPath.c_str() );

   mock().expectOneCall("Function::Parse").withConstPointerParameter("config", config).ignoreOtherParameters().andReturnValue(true);

   // Exercise
   Parser parser;
   bool result = parser.Parse( tempFilename, *config, true, false, std::vector<std::string>(), std::vector<std::string>{define}, error );

   // Verify
   CHECK_EQUAL( true, result );
   CHECK_EQUAL( 0, error.tellp() )

   // Cleanup
}

/*
 * Check that regeneration options are printed properly.
 */
TEST( MockGenerator, WithRegenOpts )
{
   // Prepare
   std::ostringstream output;
   const char* testRegenOpts = "####REGEN_OPTS######";


   // Exercise
   Parser parser;
   parser.GenerateMock( testRegenOpts, output );

   // Verify
   STRCMP_CONTAINS( StringFromFormat( "Generation options: %s", testRegenOpts ).asCharString(), output.str().c_str() );

   // Cleanup
}
