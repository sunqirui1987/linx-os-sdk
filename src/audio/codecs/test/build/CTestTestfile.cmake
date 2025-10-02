# CMake generated Testfile for 
# Source directory: /Users/sunqirui/gitlab/aiagent/linx-mongoose/sdk/codecs/test
# Build directory: /Users/sunqirui/gitlab/aiagent/linx-mongoose/sdk/codecs/test/build
# 
# This file includes the relevant testing commands required for 
# testing this directory and lists subdirectories to be tested as well.
add_test(codec_basic_test "/Users/sunqirui/gitlab/aiagent/linx-mongoose/sdk/codecs/test/build/codec_test")
set_tests_properties(codec_basic_test PROPERTIES  TIMEOUT "30" WORKING_DIRECTORY "/Users/sunqirui/gitlab/aiagent/linx-mongoose/sdk/codecs/test/build" _BACKTRACE_TRIPLES "/Users/sunqirui/gitlab/aiagent/linx-mongoose/sdk/codecs/test/CMakeLists.txt;62;add_test;/Users/sunqirui/gitlab/aiagent/linx-mongoose/sdk/codecs/test/CMakeLists.txt;0;")
subdirs("codecs")
