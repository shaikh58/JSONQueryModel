#include <iostream>
#include <fstream>
#include <functional>
#include <map>
#include "JSONParser.h"
#include "AutoGrader.h"
#include "Testable.h"
#include "Debug.h"

// STUDENT: Be sure to update this path if necessary (should point to the repo directory)
inline std::string getWorkingDirectoryPath() {
//    return "/Users/mustafa/ece141/pa3-json-processor-shaikh58";
    return "/pa3-json-processor-shaikh58";
}


bool runAutoGraderTest(const std::string& aPath, const std::string& aTestName) {
    ECE141::AutoGrader autoGrader(aPath);
    return autoGrader.runTest(aTestName);
}

bool runNoFilterTest(const std::string& aPath) {
    return runAutoGraderTest(aPath, "NoFilterTest");
}

bool runBasicTest(const std::string& aPath) {
    return runAutoGraderTest(aPath, "BasicTest");
}

bool runAdvancedTest(const std::string& aPath) {
    return runAutoGraderTest(aPath, "AdvancedTest");
}

bool runMyTest(const std::string& aPath){
    std::fstream theJsonFile(aPath + "/Resources/classroom.json");
    ECE141::Model theModel;
    ECE141::JSONParser theParser(theJsonFile);
    theParser.parse(&theModel);

    // 1) Attempting to call sum over a map is invalid and should throw a handled exception with extra info
    {
        auto theQuery = theModel.createQuery();
        try {
            theQuery.select("'location'")
                    .sum();
        }
        catch(std::invalid_argument &e){ std::cout << e.what() << std::endl; }
    }

    // 2) Attempting to call sum with non numerics inside the list should also throw an exception with extra info
    {
        auto theQuery = theModel.createQuery();
        try {
            theQuery.select("'students'")
                    .sum();
        }
        catch(std::invalid_argument &e){ std::cout << e.what() << std::endl; }
    }

    // 3) Call the Model copy constructor and run a query on that - should run as if it was a newly created Model object
    {
        ECE141::Model theSecondModel(theModel);
        theParser.parse(&theSecondModel);
        auto theQuery = theModel.createQuery();
        theQuery.select("'students'")
                .filter("index > 1")
                .count();
    }
    return true;
}

int runTest(const int argc, const char* argv[]) {
    const std::string thePath = argc > 2 ? argv[2] : getWorkingDirectoryPath();
    const std::string theTest = argv[1];

    std::map<std::string, std::function<bool(const std::string&)>> theTestFunctions { // function as a callable!
        { "compile", [](const std::string&) { return true; } },
        { "nofilter", runNoFilterTest },
        { "query", ECE141::runModelQueryTest },
        { "basic", runBasicTest },
        { "advanced", runAdvancedTest },
        {"myTest", runMyTest}
    };

    if (theTestFunctions.count(theTest) == 0) {
        std::clog << "Unkown test '" << theTest << "'\n";
        return 1;
    }

    const bool hasPassed = theTestFunctions[theTest](thePath);
    std::cout << "Test '" << theTest << "' " << (hasPassed ? "PASSED" : "FAILED") << "\n";
    return !hasPassed;
}

int main(const int argc, const char* argv[]) {
    if (argc > 1)
        return runTest(argc, argv);

    // NOTE: my own tests are integrated in the runTests() function above - enter myTest as the command
    // line argument to run them - see bool runMyTest() above for a description of each of the 3 test cases
    return 0;
}