# CMake generated Testfile for 
# Source directory: /Users/sunqirui/gitlab/aiagent/linx-os-sdk/sdk/play/test
# Build directory: /Users/sunqirui/gitlab/aiagent/linx-os-sdk/sdk/play/test/build
# 
# This file includes the relevant testing commands required for 
# testing this directory and lists subdirectories to be tested as well.
add_test(play_basic_test "/Users/sunqirui/gitlab/aiagent/linx-os-sdk/sdk/play/test/build/play_audio_test" "--basic")
set_tests_properties(play_basic_test PROPERTIES  TIMEOUT "30" WORKING_DIRECTORY "/Users/sunqirui/gitlab/aiagent/linx-os-sdk/sdk/play/test/build" _BACKTRACE_TRIPLES "/Users/sunqirui/gitlab/aiagent/linx-os-sdk/sdk/play/test/CMakeLists.txt;107;add_test;/Users/sunqirui/gitlab/aiagent/linx-os-sdk/sdk/play/test/CMakeLists.txt;0;")
subdirs("play")
subdirs("audio")
subdirs("codecs")
