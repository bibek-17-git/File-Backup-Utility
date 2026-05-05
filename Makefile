# ============================================================================
# Smart Backup Utility - MVC + SOLID Edition
# Makefile for multiple files
# ============================================================================

CXX      := g++
CXXFLAGS := -std=c++17 -Wall -Wextra -I. $(shell pkg-config --cflags gtk+-3.0)
LDFLAGS  := $(shell pkg-config --libs gtk+-3.0)
TARGET   := smart_backup_mvc

# ============================================================================
# Source files (all .cpp files)
# ============================================================================

SRCS := main.cpp \
        model/BackupModel.cpp \
        model/SettingsModel.cpp \
        view/MainView.cpp \
        controller/BackupController.cpp \
        controller/SettingsController.cpp

# Object files
OBJS := $(SRCS:.cpp=.o)

# Header files (dependencies)
HEADERS := model/IBackupModel.h \
           model/ISettingsModel.h \
           view/IMainView.h \
           view/SettingsDialog.h \
           strategies/SimpleCopyStrategy.h \
           core/BackupManager.h

# ============================================================================
# Build rules
# ============================================================================

all: $(TARGET)

# Link object files to create executable
$(TARGET): $(OBJS)
	@echo "🔗 Linking $@..."
	$(CXX) $(OBJS) -o $@ $(LDFLAGS)
	@echo "✅ Build complete: $(TARGET)"

# Compile C++ files to object files
%.o: %.cpp $(HEADERS)
	@echo "⚙️  Compiling $<..."
	$(CXX) $(CXXFLAGS) -c $< -o $@

# ============================================================================
# Utility targets
# ============================================================================

# Clean build artifacts
clean:
	@echo "🧹 Cleaning..."
	rm -f $(OBJS) $(TARGET) $(TARGET).exe
	@echo "✅ Clean complete"

# Run the program
run: $(TARGET)
	@echo "🚀 Running $(TARGET)..."
	./$(TARGET)

# Run with valgrind (memory check)
valgrind: $(TARGET)
	valgrind --leak-check=full ./$(TARGET)

# Show source files and objects
info:
	@echo "📁 Source files:"
	@echo "   $(SRCS)"
	@echo ""
	@echo "📄 Object files:"
	@echo "   $(OBJS)"
	@echo ""
	@echo "📚 Header files:"
	@echo "   $(HEADERS)"

# Create directory structure (if not exists)
dirs:
	@mkdir -p model view controller strategies core

# ============================================================================
# Platform specific settings
# ============================================================================

# For Windows (MSYS2/MinGW)
ifeq ($(OS),Windows_NT)
    CXXFLAGS += -DWIN32
    LDFLAGS  += -lole32 -lshell32 -lcomdlg32
endif

# For Linux
ifeq ($(shell uname),Linux)
    CXXFLAGS += -Dlinux
endif

# For macOS
ifeq ($(shell uname),Darwin)
    CXXFLAGS += -DmacOS
endif

# ============================================================================
# Phony targets
# ============================================================================

.PHONY: all clean run valgrind info dirs
