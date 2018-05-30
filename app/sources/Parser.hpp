#ifndef CPPUMOCKGEN_MOCKGENERATOR_HPP_
#define CPPUMOCKGEN_MOCKGENERATOR_HPP_

#include <vector>
#include <string>
#include <sstream>
#include <memory>

#include "Function.hpp"

class Config;

class Parser
{
public:
    /**
     * Parses the C/C++ header located in @p inputFilename.
     *
     * @param inputFilepath [in] Filename for the C/C++ header file
     * @param config [in] Configuration to be used during mock generation
     * @param interpretAsCpp [in] Forces interpreting the header file as C++
     * @param includePaths [in] List of paths to search for included header files
     * @param preprocessorDefines [in] List of preprocessor macro definitions
     * @param error [out] Stream where errors will be written
     * @return @c true if the input file could be parsed successfully, @c false otherwise
     */
    bool Parse( const std::string &inputFilepath, const Config &config, bool interpretAsCpp, bool useCpp11,
                const std::vector<std::string> &includePaths, const std::vector<std::string> &includeFiles, 
                std::ostream &error );

    /**
     * Generates mocked functions for the C/C++ header parsed previously.
     *
     * @param genOpts [in] String containing the generation options
     * @param output [out] Stream where the generated mocks will be written
     */
    void GenerateMock( const std::string &genOpts, std::ostream &output ) const;

    /**
     * Generates expectation functions header for the C/C++ header parsed previously.
     *
     * @param genOpts [in] String containing the generation options
     * @param output [out] Stream where the generated expectations header will be written
     */
    void GenerateExpectationHeader( const std::string &genOpts, std::ostream &output ) const;

    /**
     * Generates expectation functions implementation for the C/C++ header parsed previously.
     *
     * @param genOpts [in] String containing the generation options
     * @param headerFilepath [in] Filename for the expectation functions header file
     * @param output [out] Stream where the generated expectations implementation will be written
     */
    void GenerateExpectationImpl( const std::string &genOpts, const std::string &headerFilepath, std::ostream &output ) const;

private:
    void GenerateFileHeading( const std::string &genOpts, std::ostream &output ) const;

    std::vector<std::unique_ptr<const Function>> m_functions;
    std::string m_inputFilepath;
    bool m_interpretAsCpp;
};

#endif // header guard
