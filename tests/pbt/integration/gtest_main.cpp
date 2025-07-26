#include <gtest/gtest.h>
#include "gtest_adapter.hpp"

int main(int argc, char** argv) {
    ::testing::InitGoogleTest(&argc, argv);
    
    // Install PBT test listener for enhanced reporting
    jwwlib::pbt::InstallPBTTestListener();
    
    // Parse additional PBT-specific arguments
    for (int i = 1; i < argc; i++) {
        std::string arg(argv[i]);
        
        if (arg.find("--pbt-max-success=") == 0) {
            setenv("RC_MAX_SUCCESS", arg.substr(18).c_str(), 1);
        } else if (arg.find("--pbt-max-size=") == 0) {
            setenv("RC_MAX_SIZE", arg.substr(15).c_str(), 1);
        } else if (arg.find("--pbt-seed=") == 0) {
            setenv("RC_SEED", arg.substr(11).c_str(), 1);
        }
    }
    
    return RUN_ALL_TESTS();
}