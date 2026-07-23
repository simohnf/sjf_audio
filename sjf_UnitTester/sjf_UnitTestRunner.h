/*
███████╗     ██╗███████╗    █████╗ ██╗   ██╗██████╗ ██╗ ██████╗
██╔════╝     ██║██╔════╝   ██╔══██╗██║   ██║██╔══██╗██║██╔═══██╗
███████╗     ██║█████╗     ███████║██║   ██║██║  ██║██║██║   ██║
╚════██║██   ██║██╔══╝     ██╔══██║██║   ██║██║  ██║██║██║   ██║
███████║╚█████╔╝██║███████╗██║  ██║╚██████╔╝██████╔╝██║╚██████╔╝
╚══════╝ ╚════╝ ╚═╝╚══════╝╚═╝  ╚═╝ ╚═════╝ ╚═════╝ ╚═╝ ╚═════╝
 */
//
// Created by Simon Fay on 22/07/2026.
//

#pragma once
#include <JuceHeader.h>


namespace sjf::tests
{
class UnitTestRunner : public juce::UnitTestRunner
{
public:

    /**
     * Runs all registered JUCE unit tests (or a specific category)
     * and returns an integer exit code (0 for success, 1 for failures).
     */
    int runAndReport(const juce::String& category = {}, const int64 randomSeed = 0)
    {
        if (category.isNotEmpty())
            runTestsInCategory(category, randomSeed);
        else
            runAllTests(randomSeed);

        int totalFailures = 0;
        int totalTests = getNumResults();

        std::cout << "\n==================================================\n";
        std::cout << "               UNIT TEST RESULTS                  \n";
        std::cout << "==================================================\n";

        for (int i = 0; i < totalTests; ++i)
        {
            if (const auto* result = getResult(i))
            {
                totalFailures += result->failures;
            	auto resultString = result->unitTestName + " | " + result->subcategoryName + " | " + (result->endTime - result->startTime).getDescription();
                if (result->failures > 0)
                {
                    std::cout << "[ FAIL ] " << resultString
                              << " (" << result->failures << " failure(s))\n";

                    for (const auto& message : result->messages)
                        std::cout << "         -> " << message.toStdString() << "\n";
                }
                else
                {
                    std::cout << "[ PASS ] " << resultString << "\n";
                }
            }
        }

        std::cout << "--------------------------------------------------\n";
        if (totalFailures == 0)
        {
            std::cout << "STATUS: ALL TESTS PASSED (" << totalTests << " suite(s))\n";
        }
        else
        {
            std::cout << "STATUS: " << totalFailures << " FAILURE(S) DETECTED\n";
        }
        std::cout << "==================================================\n\n";

        return (totalFailures == 0) ? 0 : 1;
    }

protected:
    void logMessage(const juce::String& message) override
    {
        std::cout << message.toStdString() << std::endl;
    }
};
}



//DUMMY_PLUGIN_SJF_UNITTESTRUNNER_H
