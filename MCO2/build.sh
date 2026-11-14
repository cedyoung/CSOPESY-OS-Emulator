echo "Building FCFS Scheduler..."

xcrun --sdk macosx clang++ -std=c++17 \
-isysroot "$(xcrun --sdk macosx --show-sdk-path)" \
-isystem "$(xcrun --sdk macosx --show-sdk-path)/usr/include/c++/v1" \
main.cpp \
scheduler.cpp \
process.cpp \
config.cpp \
instruction.cpp \
-pthread \
-o mco2

if [ $? -ne 0 ]; then
    echo
    echo "BUILD FAILED."
    exit 1
fi

echo
echo "BUILD SUCCESSFUL."
echo

./mco2